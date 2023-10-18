// Copyright (C)
/*
 * Developed by HDH
 * Begin at 2023/6/6
*/

#include "CoRe.h"
#include "inet/common/INETDefs.h"

namespace inet {

Define_Module(CoRe);

void CoRe::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL){
        //statistics
        outGate = gate("lowerOut");
        inGate = gate("lowerIn");
        upGate = gate("upperOut");
        downGate = gate("upperIn");
        // configuration
        activate = par("activate");
        stopTime = par("stopTime");
        linkspeed = par("linkspeed");
        max_pck_size = par("max_pck_size");
        baseRTT = par("baseRTT");
        PARA_P = par("PARA_P");
        PARA_K = par("PARA_K");
        m = par("m");

        rttbytes = baseRTT.dbl()*linkspeed/8;
        max_rate = 0.05*linkspeed;

        receiver_flowMap.clear();
        sender_flowMap.clear();
        sender_StateMap.clear();
        receiver_StateMap.clear();

        currentRTTVector.setName("currentRTT (s)");
        targetVector.setName("target_delay (s)");
        mpdVector.setName("mpd (s)");
        currateVector.setName("currate");
        ratioVector.setName("ratio");
        ecnVector.setName("ecn");
        ecnratioVector.setName("ecnratio");
        ECNaVector.setName("ECNa");
    }else if (stage == INITSTAGE_TRANSPORT_LAYER) {
        registerService(Protocol::udp, gate("upperIn"), gate("upperOut"));
        registerProtocol(Protocol::udp, gate("lowerOut"), gate("lowerIn"));
    }
}

void CoRe::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        handleSelfMessage(msg);
    else
    {
        if (msg->arrivedOn("upperIn"))
        {
            processUpperpck(check_and_cast<Packet*>(msg));
        }
        else if (msg->arrivedOn("lowerIn"))
        {
            processLowerpck(check_and_cast<Packet*>(msg));
        }
    }
}

void CoRe::handleSelfMessage(cMessage *pck)
{
    TimerMsg *timer = check_and_cast<TimerMsg *>(pck);
    // Process different self-messages (timer signals)
    EV_TRACE << "Self-message " << timer << " received\n";

    switch (timer->getKind()) {
        case SENDCRED:
            send_credit(timer->getDestAddr());
            break;
        case SENDRESEND:
            send_resend(timer->getFlowId(), timer->getSeq());
            break;
    }
}


void CoRe::processUpperpck(Packet *pck)
{
    if (string(pck->getFullName()).find("Data") != string::npos && activate==true)
    {
        int flowid;
        simtime_t cretime;
        int priority;
        uint messageLength;
        for (auto& region : pck->peekData()->getAllTags<HiTag>()){
            flowid = region.getTag()->getFlowId();
            cretime = region.getTag()->getCreationtime();
            priority = region.getTag()->getPriority();
            messageLength = region.getTag()->getFlowSize();
        }
        if(sender_flowMap.find(flowid)==sender_flowMap.end()){
            sender_flowinfo snd_info;

            auto addressReq = pck->addTagIfAbsent<L3AddressReq>();
            auto udpHeader = pck->peekAtFront<UdpHeader>();
            srcAddr = addressReq->getSrcAddress();
            L3Address destAddr = addressReq->getDestAddress();
            snd_info.flowid = flowid;
            snd_info.flowsize = messageLength;
            snd_info.cretime = cretime;
            snd_info.destAddr = destAddr;
            snd_info.srcPort = udpHeader->getSrcPort();
            snd_info.destPort = udpHeader->getDestPort();
            snd_info.pckseq = 0;
            snd_info.priority = priority;
            snd_info.crcMode = udpHeader->getCrcMode();
            snd_info.crc = udpHeader->getCrc();
            if(sender_StateMap.find(destAddr)==sender_StateMap.end()||sender_StateMap[destAddr]==CSTOP_SENT){
                sendRtt = 0;
                if(snd_info.flowsize < rttbytes)
                {
                    unsSize = snd_info.flowsize;
                    sSize = 0;
                }
                else
                {
                    unsSize = rttbytes;
                    sSize = snd_info.flowsize - rttbytes;
                }

                sender_flowMap[flowid] = snd_info;

                send_credreq(flowid);
                send_unschedule(flowid);
            }else{
                sender_flowMap[flowid] = snd_info;
                sSize+=messageLength;
            }

            EV << "store new flow, id = "<<snd_info.flowid<<", size = " << snd_info.flowsize <<
                    ", creationtime = "<<snd_info.cretime<<endl;
        }
        delete pck;
    }
    else
    {
        EV<<"Unknown packet, sendDown()."<<endl;
        sendDown(pck);
    }
}

void CoRe::processLowerpck(Packet *pck)
{
    if (!strcmp(pck->getFullName(),"credit_req"))
    {
        receive_credreq(pck);
    }
    else if (!strcmp(pck->getFullName(),"credit"))
    {
        receive_credit(pck);
    }
    else if (!strcmp(pck->getFullName(),"Resend"))
    {
        receive_resend(pck);
    }
    else if (string(pck->getFullName()).find("CoRe") != string::npos)
    {
        receive_data(pck);
    }
    else
    {
        sendUp(pck);
    }
}

void CoRe::send_credreq(uint32_t flowid)
{
    sender_flowinfo snd_info = sender_flowMap.find(flowid)->second;
    sender_StateMap[snd_info.destAddr] = CREDIT_RECEIVING;
    Packet *cred_req = new Packet("credit_req");
    const auto& payload = makeShared<ByteCountChunk>(B(26));
    auto tag = payload->addTag<HiTag>();
    tag->setFlowId(flowid);
    payload->enableImplicitChunkSerialization = true;
    cred_req->insertAtBack(payload);


    cred_req->addTagIfAbsent<L3AddressReq>()->setDestAddress(snd_info.destAddr);
    cred_req->addTagIfAbsent<L3AddressReq>()->setSrcAddress(srcAddr);
    cred_req->setTimestamp(simTime());

    cred_req->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    cred_req->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

    EV << "send creditreq to "<<snd_info.destAddr.toIpv4()<<", dest state = "<<sender_StateMap[snd_info.destAddr]<<endl;
    sendDown(cred_req); // send down the credit_request.
}

void CoRe::send_unschedule(uint32_t flowid)
{
    sender_flowinfo snd_info = sender_flowMap.find(flowid)->second;
    std::ostringstream str;

    str << "CoReUData" << "-" <<snd_info.flowid<< "-" <<snd_info.pckseq;
    Packet *packet = new Packet(str.str().c_str());
    int64_t this_pck_bytes = (sendRtt+max_pck_size<=unsSize)?max_pck_size:unsSize-sendRtt;
    sendRtt += this_pck_bytes;
    sender_flowMap[flowid].flowsize -= this_pck_bytes;

    const auto& payload = makeShared<ByteCountChunk>(B(this_pck_bytes));
    auto tag = payload->addTag<HiTag>();
    if(sender_flowMap[flowid].flowsize==0) tag->setIsLastPck(true);
    tag->setFlowId(snd_info.flowid);
    tag->setPacketId(snd_info.pckseq);
    if(lastflowid!=snd_info.flowid){
        last_creation_time = snd_info.cretime;
        lastflowid = snd_info.flowid;
    }
    tag->setCreationtime(last_creation_time);
    tag->setPriority(snd_info.priority);
    tag->setFlowSize(snd_info.flowsize);
    packet->insertAtBack(payload);
    snd_info.pckseq++;

    auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
    addressReq->setSrcAddress(srcAddr);
    addressReq->setDestAddress(snd_info.destAddr);

    const Protocol *l3Protocol = &Protocol::ipv4;
    auto udpHeader = makeShared<UdpHeader>();
    udpHeader->setSourcePort(snd_info.srcPort);
    udpHeader->setDestinationPort(snd_info.destPort);
    udpHeader->setCrc(snd_info.crc);
    udpHeader->setCrcMode(snd_info.crcMode);
    udpHeader->setTotalLengthField(udpHeader->getChunkLength() + packet->getTotalLength());
    insertTransportProtocolHeader(packet, Protocol::udp, udpHeader);

//    //insert POSEIDON header(INTHeader)
    auto content = makeShared<PSDINTHeader>();
    content->setTS(simTime());
    content->enableImplicitChunkSerialization = true;
    packet->insertAtFront(content);

    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);
    packet->addTagIfAbsent<EcnReq>()->setExplicitCongestionNotification(1);
    packet->setKind(0);

    EV << "prepare to send unschedule packet, remaining unschedule data size = " << unsSize-sendRtt <<endl;
    sendDown(packet);
    sender_flowMap[flowid] = snd_info;
    if (sendRtt < unsSize)
    {
        send_unschedule(flowid);
    }

}

void CoRe::receive_credreq(Packet *pck)
{
    currentRTT = simTime();
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    srcAddr = l3addr->getDestAddress();
    if(receiver_StateMap.find(l3addr->getSrcAddress())==receiver_StateMap.end()){
        receiver_StateMap[l3addr->getSrcAddress()]=CREDIT_STOP;
    }

    if(receiver_StateMap[l3addr->getSrcAddress()]==CREDIT_SENDING){
        delete pck;
    }else{
        receiver_flowinfo rcvflow;
        if(receiver_flowMap.find(l3addr->getSrcAddress())==receiver_flowMap.end()){
            rcvflow.destAddr=l3addr->getSrcAddress();
            rcvflow.now_send_cdt_seq = 0;
            rcvflow.current_speed = linkspeed*0.05;
            rcvflow.last_update_time = simTime();
            rcvflow.pck_in_rtt=0;
            rcvflow.ecn_in_rtt=0;
            rcvflow.ReceiverState=CREDIT_SENDING;
            rcvflow.sendcredit = new TimerMsg("sendcredit");
            rcvflow.sendcredit->setKind(SENDCRED);
            rcvflow.sendcredit->setDestAddr(l3addr->getSrcAddress());
            lasttime=simTime();
            receiver_flowMap[l3addr->getSrcAddress()]=rcvflow;
        }else{
            rcvflow = receiver_flowMap[l3addr->getSrcAddress()];
        }

//        cancelEvent(rcvflow.sendcredit);
//        scheduleAt(simTime(),rcvflow.sendcredit);

        receiver_StateMap[l3addr->getSrcAddress()]=CREDIT_SENDING;
        send_credit(l3addr->getSrcAddress());
        delete pck;
    }
}

void CoRe::send_credit(L3Address destaddr)
{
    Packet *credit = new Packet("credit");

    receiver_flowinfo rcvflow = receiver_flowMap[destaddr];
    int jitter_bytes = intrand(8)+1;
    const auto& content = makeShared<ByteCountChunk>(B(credit_size/8+jitter_bytes));
    auto tag = content->addTag<HiTag>();
    tag->setPacketId(rcvflow.now_send_cdt_seq);
    content->enableImplicitChunkSerialization = true;

    credit->insertAtBack(content);

    credit->setTimestamp(simTime());
    credit->addTagIfAbsent<L3AddressReq>()->setDestAddress(rcvflow.destAddr);
    credit->addTagIfAbsent<L3AddressReq>()->setSrcAddress(srcAddr);
    credit->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    credit->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

    send(credit, outGate);
    EV<<"send credit "<<rcvflow.now_send_cdt_seq<<endl;
    rcvflow.now_send_cdt_seq++;
    receiver_flowMap[destaddr] = rcvflow;
//    bitlength+= 84*8;
//    if(simTime()-lasttime>=0.0001){
//        currentRate = bitlength/(simTime()-lasttime).dbl();
//        currateVector.recordWithTimestamp(simTime(),currentRate);
//        bitlength = 0;
//        lasttime = simTime();
//    }
    if(receiver_StateMap[destaddr]==CREDIT_SENDING&&simTime()<stopTime){
        rcvflow.sendcredit->setDestAddr(destaddr);
        simtime_t interval = (simtime_t)((credit_size+(jitter_bytes+46)*8)/rcvflow.current_speed+12*8/linkspeed);
        //58=20(IP)+14(EthernetMac)+8(EthernetPhy)+4(EthernetFcs)+12(interframe gap,IFG)
        scheduleAt(simTime() + interval,rcvflow.sendcredit);
        EV<<"currate = "<<rcvflow.current_speed<<", jitter_bytes = "<<jitter_bytes<<", next time = "<<simTime() + interval<<"s"<<endl;
    }
}

void CoRe::receive_credit(Packet *pck)
{
    auto l3addr = pck->getTag<L3AddressInd>();
    EV<<"receive_credit(), the source "<<l3addr->getSrcAddress()<<endl;

    long packetid;
    for (auto& region : pck->peekData()->getAllTags<HiTag>())
    {
        packetid = region.getTag()->getPacketId();
        EV <<"credit_id = "<< packetid <<endl;
    }

    // if no flow exits to be transmitted, stop transmitting.
    if (simTime()>=stopTime||sSize == 0 )
    {
        // send stop credit to the receiver
        cancelEvent(sendstop);
        sendstop->setKind(SENDSTOP);
        sendstop->setDestAddr(l3addr->getSrcAddress());
        EV<<"receive_credit(), remainsize = 0"<<endl;
        scheduleAt(simTime() + timeout,sendstop);
    }
    else
    {
        // none stop
        sender_flowinfo snd_info= sender_flowMap.begin()->second;
        int flowid =sender_flowMap.begin()->first;
        std::ostringstream str;
        str << packetName << "-" <<flowid<<"-"<<packetid;
        Packet *packet = new Packet(str.str().c_str());
        int packetLength;
        bool last = false;
        if (snd_info.flowsize >= max_pck_size)
        {
            sSize -=  max_pck_size;
            sender_flowMap[flowid].flowsize-=  max_pck_size;
            packetLength=max_pck_size;
        }
        else if(sSize>=max_pck_size)
        {
            last = true;
            sender_flowMap.erase(flowid);
            sender_flowMap.begin()->second.flowsize-=(max_pck_size-snd_info.flowsize);
            sSize -= max_pck_size;
            packetLength=max_pck_size;
        }
        else {
            packetLength=max_pck_size;
            sSize=0;
        }
        const auto& payload = makeShared<ByteCountChunk>(B(packetLength));
        auto tag = payload->addTag<HiTag>();
        tag->setReverse(true);
        tag->setFlowId(flowid);
        tag->setPacketId(packetid);
        tag->setPriority(snd_info.priority);
        if(lastflowid!=snd_info.flowid){
            last_creation_time = snd_info.cretime;
            lastflowid = snd_info.flowid;
        }
        tag->setCreationtime(last_creation_time);
        if(last)
            tag->setIsLastPck(true);

        packet->insertAtBack(payload);

        auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
        addressReq->setSrcAddress(l3addr->getDestAddress());
        addressReq->setDestAddress(l3addr->getSrcAddress());

        // insert udpHeader, set source and destination port
        const Protocol *l3Protocol = &Protocol::ipv4;
        auto udpHeader = makeShared<UdpHeader>();
        udpHeader->setSourcePort(snd_info.srcPort);
        udpHeader->setDestinationPort(snd_info.destPort);
        udpHeader->setCrc(snd_info.crc);
        udpHeader->setCrcMode(snd_info.crcMode);
        udpHeader->setTotalLengthField(udpHeader->getChunkLength() + packet->getTotalLength());
        insertTransportProtocolHeader(packet, Protocol::udp, udpHeader);

        //insert POSEIDON header(INTHeader)
        auto content = makeShared<PSDINTHeader>();
        content->setTS(simTime());
        content->enableImplicitChunkSerialization = true;
        packet->insertAtFront(content);

        packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);
        packet->addTagIfAbsent<EcnReq>()->setExplicitCongestionNotification(1);
        packet->setKind(0);
        packet->setTimestamp(pck->getTimestamp());

        EV << "prepare to send packet, remaining data size = " << sSize <<endl;
        sendDown(packet);
    }
    delete pck;
}

void CoRe::receive_data(Packet *pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    auto it = receiver_flowMap.find(l3addr->getSrcAddress());
    receiver_flowinfo tinfo = it->second;
    int ackid;
    simtime_t ts = pck->getTimestamp();
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        ackid = region.getTag()->getPacketId();
    }
//    auto ecn = pck->addTagIfAbsent<EcnInd>()->getExplicitCongestionNotification();
//    if(ecn==3)
//        tinfo.ecn_in_rtt++;
//    ecnVector.recordWithTimestamp(simTime(),ecn);
//
//    currentRTT = simTime()-ts;
//    currentRTTVector.recordWithTimestamp(simTime(),currentRTT);
    EV<<"ts = "<<ts<<", t_last_decrease = "<<t_last_decrease<<endl;
    if((ts>=t_last_decrease)){
        can_decrease = true;
    }

    const auto& INT_msg = pck->popAtFront<PSDINTHeader>();
    double mpd = INT_msg->getMPD().dbl();
//    tinfo.pck_in_rtt++;

    EV<<"currate = "<<tinfo.current_speed<<endl;

//    double ecnratio = double(tinfo.ecn_in_rtt)/double(tinfo.pck_in_rtt);
    double target = PARA_P * (log(max_rate) - log(tinfo.current_speed)) / (log(max_rate) - log(min_rate)) + PARA_K;
    double ratio = exp((target - mpd) / PARA_P * (log(max_rate) - log(min_rate)) * m);

    ratio = math::clamp(ratio,0.4,2.5);
    EV<<"ratio = "<<ratio<<", mpd = "<<mpd<<", target = "<<target<<endl;
    ratioVector.recordWithTimestamp(simTime(),ratio);
    mpdVector.recordWithTimestamp(simTime(),mpd);
    targetVector.recordWithTimestamp(simTime(),target);

//    if(ts>=tinfo.last_update_time){
//        ECN_a=(1-g)*ECN_a+g*ecnratio;
//        tinfo.last_update_time = simTime();
//        tinfo.ecn_in_rtt=0;
//        tinfo.pck_in_rtt=0;
//    }
//    if(ecn==3){
//        tinfo.current_speed=tinfo.current_speed*(1-ECN_a/2);
//    }
//    else if(ratio<=1){
//        if(can_decrease){
//            tinfo.current_speed=tinfo.current_speed*ratio;
//            t_last_decrease = simTime();
//            tinfo.pck_in_rtt=0;
//            can_decrease = false;
//       }
//    }
//    else{
//        tinfo.current_speed=tinfo.current_speed+72*8*72*8/(max_rate*currentRTT.dbl()*currentRTT.dbl());
//    }
//    ecnratioVector.recordWithTimestamp(simTime(),ecnratio);
//    ECNaVector.recordWithTimestamp(simTime(),ECN_a);



    if(mpd<=target){
        tinfo.current_speed += (ratio-1)*tinfo.current_speed;
    }else if(can_decrease){
        tinfo.current_speed = tinfo.current_speed*ratio;
        t_last_decrease = simTime();
        can_decrease = false;
    }

    tinfo.current_speed = math::clamp(tinfo.current_speed,min_rate,max_rate);
    EV<<"after changing, currate = "<<tinfo.current_speed<<endl;
    currateVector.recordWithTimestamp(simTime(), tinfo.current_speed);
    receiver_flowMap[l3addr->getSrcAddress()]=tinfo;

    sendUp(pck);
}

void CoRe::send_stop(L3Address destaddr)
{
    sender_StateMap[destaddr] = CSTOP_SENT;
    Packet *cred_stop = new Packet("credit_stop");
    cred_stop->addTagIfAbsent<L3AddressReq>()->setDestAddress(destaddr);
    cred_stop->addTagIfAbsent<L3AddressReq>()->setSrcAddress(srcAddr);
    cred_stop->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    cred_stop->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

    EV << "send_stop(), DestAddr = "<<destaddr.toIpv4()<<endl;
    send(cred_stop, outGate);// send down the credit_stop.
}

void CoRe::receive_stopcred(Packet *pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    EV<<"stop credit sending for "<<l3addr->getSrcAddress()<<endl;
    receiver_StateMap[l3addr->getSrcAddress()]=CREDIT_STOP;

    delete pck;
}


void CoRe::send_resend(uint32_t flowid, long seq)
{
//    receiver_flowinfo rcv_info = receiver_flowMap[flowid];
//    Packet *resend = new Packet("Resend");
//
//    const auto& content = makeShared<ByteCountChunk>(B(1));
//    auto tag = content -> addTag<HiTag>();
//    tag->setPriority(0);
//    tag->setFlowId(rcv_info.flowid);
//    //此时packetid为期待重传的包
//    tag->setPacketId(seq);
//    resend->insertAtFront(content);
//
//    resend->addTagIfAbsent<L3AddressReq>()->setDestAddress(rcv_info.destAddr);
//    resend->addTagIfAbsent<L3AddressReq>()->setSrcAddress(srcAddr);
//    resend->setTimestamp(simTime());
//    resend->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
//    resend->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);
//
//    sendDown(resend);
//    EV << "send resend, flow_id = " << rcv_info.flowid << ", the miss pckid = " << seq << ", timestamp = " << simTime() << endl;
}

void CoRe::receive_resend(Packet* pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    L3Address srcAddr = l3addr->getDestAddress();
    L3Address destAddr = l3addr->getSrcAddress();

    int seq;
    int flowid;
    for (auto& region : pck->peekData()->getAllTags<HiTag>())
    {
       seq = region.getTag()->getPacketId();
       flowid = region.getTag()->getFlowId();
    }
    sender_flowinfo snd_info = sender_flowMap.find(flowid) -> second;

    std::ostringstream str;
    str << "ScheduleData" << "-" << flowid << "-" <<seq;
    Packet *packet = new Packet(str.str().c_str());
    int64_t this_pck_bytes = max_pck_size;

    const auto& payload = makeShared<ByteCountChunk>(B(this_pck_bytes));
    auto tag = payload->addTag<HiTag>();
    tag->setFlowId(snd_info.flowid);
    tag->setPacketId(seq);
    tag->setCreationtime(simTime());
    tag->setPriority(7);
    packet->insertAtBack(payload);

    auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
    addressReq->setSrcAddress(srcAddr);
    addressReq->setDestAddress(snd_info.destAddr);
    packet->setTimestamp(simTime());
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);
    packet->setKind(0);

    EV << "resend pckid = " << seq << endl;
    sendDown(packet);

}

void CoRe::sendDown(Packet *pck)
{
    EV << "sendDown()" <<endl;
    send(pck,outGate);
}

void CoRe::sendUp(Packet *pck)
{
    EV<<"CoRe, oh sendup!"<<endl;
    send(pck,upGate);
}

void CoRe::refreshDisplay() const
{

}

void CoRe::finish()
{

}
}//namespace inet;


