// Copyright (C)
/*
 * Developed by Angrydudu-gy
 * Begin at 2020/04/28
*/
#include "HPCC.h"

namespace inet {

Define_Module(HPCC);

void HPCC::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL){
        //gates
        lowerOutGate = gate("lowerOut");
        lowerInGate = gate("lowerIn");
        upperOutGate = gate("upperOut");
        upperInGate = gate("upperIn");
        // configuration
        stopTime = par("stopTime");
        activate = par("activate");
        linkspeed = par("linkspeed");
        max_pck_size=par("max_pck_size");
        baseRTT = par("baseRTT");
        expectedFlows = par("expectedFlows");

        wint = linkspeed*baseRTT.dbl();
        wai = wint*(1-yita)/expectedFlows;
        currentRate = linkspeed;
        csend_window = wint;
        send_window = wint;
        Usignal = cComponent::registerSignal("Usignal");

        senddata = new TimerMsg("senddata");
        senddata->setKind(SENDDATA);

    }else if (stage == INITSTAGE_TRANSPORT_LAYER) {
        registerService(Protocol::udp, gate("upperIn"), gate("upperOut"));
        registerProtocol(Protocol::udp, gate("lowerOut"), gate("lowerIn"));
//        registerService(Protocol::hpcc, gate("upperIn"), gate("upperOut"));
//        registerProtocol(Protocol::hpcc, gate("lowerOut"), gate("lowerIn"));
    }

}

void HPCC::handleMessage(cMessage *msg)
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

void HPCC::handleSelfMessage(cMessage *pck)
{
    // Process different self-messages (timer signals)
    TimerMsg *timer = check_and_cast<TimerMsg *>(pck);
    EV_TRACE << "Self-message " << timer << " received, type = "<<timer->getKind()<<endl;
    switch (timer->getKind()) {
        case SENDDATA:
        {
            if(simTime()>=stopTime){
                SenderState=STOPPING;
                break;
            }
            else{
                send_data();
                break;
            }
        }
    }
}

// Record the packet from app to transmit it to the dest
void HPCC::processUpperpck(Packet *pck)
{
    if (string(pck->getFullName()).find("Data") != string::npos&&activate==true){
        int flowid;
        simtime_t cretime;
        int priority;
        for (auto& region : pck->peekData()->getAllTags<HiTag>()){
            flowid = region.getTag()->getFlowId();
            cretime = region.getTag()->getCreationtime();
            priority = region.getTag()->getPriority();
        }
        EV<<"the flow id is "<<flowid<<endl;
        auto addressReq = pck->addTagIfAbsent<L3AddressReq>();
        L3Address srcAddr = addressReq->getSrcAddress();
        L3Address destAddr = addressReq->getDestAddress();
        EV<<"the src add is "<<srcAddr<<" the des add  is "<<destAddr<<endl;
        auto udpHeader = pck->removeAtFront<UdpHeader>();
        int lastPckLen = pck->getByteLength() % max_pck_size;
        int maxPckNum;
        if (lastPckLen == 0)//if the flow can be divided with no remainder
        {
            lastPckLen = max_pck_size;
            maxPckNum = pck->getByteLength() / max_pck_size;
        }
        else
        {
            maxPckNum = (pck->getByteLength() / max_pck_size) + 1;
        }
        EV<<"the max pck num is "<<maxPckNum<<endl;

        for(int i=0;i<maxPckNum;i++){
            sender_packetinfo snd_info;

            snd_info.srcAddr = srcAddr;
            snd_info.destAddr = destAddr;
            snd_info.srcPort = udpHeader->getSrcPort();
            snd_info.destPort = udpHeader->getDestPort();
            snd_info.flowid = flowid;
            snd_info.crcMode = udpHeader->getCrcMode();
            snd_info.crc = udpHeader->getCrc();
            snd_info.cretime = cretime;
            snd_info.priority = priority;
            if(i==maxPckNum-1){
                snd_info.length = lastPckLen;
                snd_info.last = true;
            }else{
                snd_info.length = max_pck_size;
            }

            sender_packetMap[packetid]=snd_info;
            if(SenderState==STOPPING){
                SenderState=SENDING;
                scheduleAt(simTime(),senddata);
            }
            packetid++;
        }
        delete pck;
    }
    else
    {
        EV<<"Unknown packet, sendDown()."<<endl;
        sendDown(pck);
    }
}

void HPCC::send_data()
{
    int packetid = nxtSendpacketid;
    sender_packetinfo snd_info = sender_packetMap.find(packetid)->second;
    EV<<"send_data(), prepare to send packet to destination "<<snd_info.destAddr.toIpv4()<<endl;
    std::ostringstream str;
    str << packetName << "-" <<packetid;
    Packet *packet = new Packet(str.str().c_str());
    const auto& payload = makeShared<ByteCountChunk>(B(snd_info.length));
    auto tag = payload->addTag<HiTag>();
    tag->setFlowId(snd_info.flowid);
    tag->setPriority(snd_info.priority);
    tag->setCreationtime(snd_info.cretime);
    tag->setPacketId(packetid);
    tag->setIsLastPck(snd_info.last);

    packet->insertAtBack(payload);

    //generate and insert a new udpHeader, set source and destination port
    const Protocol *l3Protocol = &Protocol::ipv4;
    auto udpHeader = makeShared<UdpHeader>();
    udpHeader->setSourcePort(snd_info.srcPort);
    udpHeader->setDestinationPort(snd_info.destPort);
    udpHeader->setCrc(snd_info.crc);
    udpHeader->setCrcMode(snd_info.crcMode);
    udpHeader->setTotalLengthField(B(udpHeader->getChunkLength()+packet->getTotalLength()));
    insertTransportProtocolHeader(packet, Protocol::udp, udpHeader);

    //insert HPCC header(INTHeader)
    auto content = makeShared<INTHeader>();
    content->setNHop(0);
    content->setPathID(0);
    content->enableImplicitChunkSerialization = true;
    packet->insertAtFront(content);

    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);
    packet->setKind(0);
    auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
    addressReq->setSrcAddress(snd_info.srcAddr);
    addressReq->setDestAddress(snd_info.destAddr);

    EV<<"packet length = "<<packet->getByteLength()<<", current rate = "<<currentRate<<endl;
    EV<<"packet name "<<packet->getFullName()<<endl;

    sendDown(packet);
    send_window-=snd_info.length;
    nxtSendpacketid+=1;

    if(sender_packetMap.find(nxtSendpacketid)==sender_packetMap.end()){
        EV<<"packet run out, stopping"<<endl;
        SenderState = STOPPING;
    }
    else if(send_window>=sender_packetMap[nxtSendpacketid].length){
        simtime_t nxtSendTime = simtime_t((packet->getByteLength()+58)*8/currentRate) + simTime();
        //58=20(IP)+14(EthernetMac)+8(EthernetPhy)+4(EthernetFcs)+12(interframe gap,IFG)
        scheduleAt(nxtSendTime,senddata);
    }
    else{
        EV<<"Pausing !!!!!"<<endl;
        SenderState = PAUSING;
    }

}

void HPCC::processLowerpck(Packet *pck)
{
    if (string(pck->getFullName()).find(packetName) != string::npos)//get Data packet
    {
        receive_data(pck);
    }
    else if(string(pck->getFullName()).find("ACK") != string::npos)
    {
        receiveAck(pck);
    }
    else
    {
        sendUp(pck);
    }
}

void HPCC::receive_data(Packet *pck)
{
    auto l3AddressInd = pck->getTag<L3AddressInd>();
    auto srcAddr = l3AddressInd->getSrcAddress();//get sourceAddress
    auto desAddr = l3AddressInd->getDestAddress();//get desAddress

    const auto& INT_msg = pck->popAtFront<INTHeader>(); // pop and obtain the INTHeader
    int curRcvNum;//current received packet Serial number
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        curRcvNum = region.getTag()->getPacketId();
    }

    if(receiver_packetMap.find(srcAddr)==receiver_packetMap.end()){
        receiver_packetMap[srcAddr]=0;
        if(curRcvNum==receiver_packetMap[srcAddr]){
            receiver_packetMap[srcAddr]++;
            EV_DETAIL<<"packet is ordered, send ACK!"<<endl;
            std::ostringstream str;
            str <<"ACK-" <<curRcvNum;
            Packet *intinfo = new Packet(str.str().c_str());
            const auto& payload = makeShared<ByteCountChunk>(B(1));
            auto tag = payload->addTag<HiTag>();
            tag->setPacketId(curRcvNum);
            payload->enableImplicitChunkSerialization = true;
            intinfo->insertAtBack(payload);

            intinfo->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

            intinfo->insertAtFront(INT_msg);
            intinfo->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);

            auto addressReq = intinfo->addTagIfAbsent<L3AddressReq>();
            addressReq->setSrcAddress(desAddr);
            addressReq->setDestAddress(srcAddr);

            EV_INFO << "Sending INT from "<<desAddr<<" to "<< srcAddr <<endl;//could include more details
            EV_DETAIL<<"INT sequence is "<<curRcvNum<<endl;
            sendDown(intinfo);
            EV_DETAIL<<"received a packet of new flow successfully, The transport path = "<<INT_msg->getPathID()<<endl;//gy
            sendUp(pck);
        }
        else{
            EV_DETAIL<<"packet is out of order, send NAK!"<<endl;
            std::ostringstream str;
            str <<"NACK-" <<receiver_packetMap[srcAddr];
            Packet *intinfo = new Packet(str.str().c_str());
            const auto& payload = makeShared<ByteCountChunk>(B(1));
            auto tag = payload->addTag<HiTag>();
            tag->setPacketId(receiver_packetMap[srcAddr]);
            payload->enableImplicitChunkSerialization = true;
            intinfo->insertAtBack(payload);

            intinfo->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

            intinfo->insertAtFront(INT_msg);
            intinfo->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);

            auto addressReq = intinfo->addTagIfAbsent<L3AddressReq>();
            addressReq->setSrcAddress(desAddr);
            addressReq->setDestAddress(srcAddr);

            EV_INFO << "Sending INT from "<<desAddr<<" to "<< srcAddr <<endl;//could include more details
            EV_DETAIL<<"INT sequence is "<<curRcvNum<<endl;
            sendDown(intinfo);//send int and inform the src go back N
            //print INTinfo
            EV_DETAIL<<"received a out of order packet of new flow, The transport path : nhop = "<<INT_msg->getNHop()<<endl;
            delete pck;
        }
    }
    else{
        if(curRcvNum == receiver_packetMap[srcAddr])//ordered
        {
            receiver_packetMap[srcAddr]++;
            EV_DETAIL<<"packet is ordered, send ACK!"<<endl;
            std::ostringstream str;
            str <<"ACK-" <<curRcvNum;
            Packet *intinfo = new Packet(str.str().c_str());
            const auto& payload = makeShared<ByteCountChunk>(B(1));
            auto tag = payload->addTag<HiTag>();
            tag->setPacketId(curRcvNum);
            payload->enableImplicitChunkSerialization = true;
            intinfo->insertAtBack(payload);

            intinfo->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

            intinfo->insertAtFront(INT_msg);
            intinfo->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);

            auto addressReq = intinfo->addTagIfAbsent<L3AddressReq>();
            addressReq->setSrcAddress(desAddr);
            addressReq->setDestAddress(srcAddr);

            EV_INFO << "Sending INT from "<<desAddr<<" to "<< srcAddr <<endl;//could include more details
            EV_DETAIL<<"INT sequence is "<<curRcvNum<<endl;
            sendDown(intinfo);
            EV_DETAIL<<"received a packet of new flow successfully, The transport path = "<<INT_msg->getPathID()<<endl;
            sendUp(pck);
        }
        else//out of order
        {
            EV_DETAIL<<"packet is out of order, send NAK!"<<endl;
            std::ostringstream str;
            str <<"NACK-" <<receiver_packetMap[srcAddr];
            Packet *intinfo = new Packet(str.str().c_str());
            const auto& payload = makeShared<ByteCountChunk>(B(1));
            auto tag = payload->addTag<HiTag>();
            tag->setPacketId(receiver_packetMap[srcAddr]);
            payload->enableImplicitChunkSerialization = true;
            intinfo->insertAtBack(payload);

            intinfo->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

            intinfo->insertAtFront(INT_msg);
            intinfo->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);

            auto addressReq = intinfo->addTagIfAbsent<L3AddressReq>();
            addressReq->setSrcAddress(desAddr);
            addressReq->setDestAddress(srcAddr);

            EV_INFO << "Sending INT from "<<desAddr<<" to "<< srcAddr <<endl;//could include more details
            EV_DETAIL<<"INT sequence is "<<curRcvNum<<endl;
            sendDown(intinfo);//send int and inform the src go back N
            //print INTinfo
            EV_DETAIL<<"received a out of order packet of new flow, The transport path : nhop = "<<INT_msg->getNHop()<<endl;
            delete pck;
        }
    }
}

void HPCC::receiveAck(Packet *pck)
{
    int ackid;
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        ackid = region.getTag()->getPacketId();
    }

    if(string(pck->getFullName()).find("NACK") != string::npos){
        EV<<"this is "<<pck->getFullName()<<endl;
        nxtSendpacketid = ackid;//go back N
        while(!senddata->isScheduled()){
            cancelEvent(senddata);
            scheduleAt(simTime(),senddata);
        }

    }else{
        sender_packetMap.erase(ackid);
    }

    const auto& INT_msg = pck->peekAtFront<INTHeader>();
    int size = INT_msg->getHopInfsArraySize();

    if(isFirstAck){
        for(int i = 0; i < size; i++)
        {
            Last[i]  = INT_msg->getHopInfs(i);
        }
        isFirstAck=false;
        delete pck;
        return;
    }
    hopInf curINTs[size];
    for(int i = 0; i < size; i++)
    {
        curINTs[i] = INT_msg->getHopInfs(i);
    }
    EV<<"the INT size is "<<size<<" the ack sequence is "<<ackid<<endl;


    //function MEASUREINFLIGHT(ack)
    double u = 0;
    simtime_t tao;
    for(int i = 0; i < size; i++)
    {
        EV<<"tx bytes is "<<curINTs[i].txBytes<<", last txbytes is "<<Last[i].txBytes<<", INT inf "<<i<<endl;
        EV<<"TS is "<<curINTs[i].TS.dbl()<<", last TS is "<<Last[i].TS.dbl()<<endl;
        double Rate = (curINTs[i].txBytes - Last[i].txBytes)*8 / (curINTs[i].TS.dbl() - Last[i].TS.dbl());
        EV<<"Rate = "<<Rate<<endl;
        EV<<"txrate is "<<curINTs[i].txRate<<", last txrate is "<<Last[i].txRate<<endl;
        EV<<"queuelength is "<<curINTs[i].queueLength<<", last queuelength is "<<Last[i].queueLength<<endl;
        double u1 = (minval(curINTs[i].queueLength, Last[i].queueLength)) /(curINTs[i].txRate * baseRTT.dbl()) + Rate/curINTs[i].txRate;
        EV<<" u1 = "<<u1<<endl;
        if(u1 > u)
        {
            u = u1;
            tao = curINTs[i].TS - Last[i].TS;
        }
    }
    EV<<"total link u ="<<u<<endl;
    if (tao > baseRTT)
        tao = baseRTT;
    U = (1-(tao.dbl()/baseRTT.dbl()))*U + (tao.dbl()/baseRTT.dbl())*u;
    emit(Usignal,U);
    EV<<"the ACK computed U is "<<U<<endl;

    EV<<"csend_window = "<<csend_window<<endl;
    //function COMPUTEWIND(U,updatewc)
    if(U >= yita || incstage >= maxstage)
    {
        //update the parameter
        send_window = csend_window/(U/yita) + wai;
        EV<<" the wai is "<<wai<<" the snd wind is "<<send_window<<endl;
        if(ackid > lastUpdateSeq)
        {
            incstage = 0;
            csend_window = send_window;
            lastUpdateSeq = nxtSendpacketid;//the next packet to send
            EV<<"update the csendwind! the last UpdateSeq is"<<lastUpdateSeq<<" the incstage is"<< incstage<<endl;
        }
    }
    else
    {
        send_window = csend_window + wai;
        if(send_window > wint)
            send_window = wint;

        EV<<" the wai is "<<wai<<" the snd wind is "<<send_window<<endl;
        if(ackid > lastUpdateSeq)
        {
            incstage++;
            EV<<"update the csendwind! the last UpdateSeq is "<<lastUpdateSeq<<" the incstage is "<< incstage<<endl;
            csend_window = send_window;
            lastUpdateSeq = nxtSendpacketid;//the next packet to send
        }
    }
    if(send_window<0)
        throw cRuntimeError("send_windows<0");

    currentRate = send_window / baseRTT.dbl();
    EV<<"the ACK computed rate is "<<currentRate<<endl;

    for(int i = 0; i < size; i++)
    {
        Last[i]  = INT_msg->getHopInfs(i);
    }

    if(SenderState==PAUSING){
        SenderState=SENDING;
        scheduleAt(simTime(),senddata);
    }
    delete pck;
}

double HPCC::minval(b numa, b numb)
{
    double result;
    if(numa < numb)
        result = numa.get();
    else
        result = numb.get();
    return double(result);
}

void HPCC::sendDown(Packet *pck)
{
    EV << "sendDown " <<pck->getFullName()<<endl;
    send(pck,lowerOutGate);
}

void HPCC::sendUp(Packet *pck)
{
    EV<<"HPCC, oh sendup!"<<endl;
    send(pck,upperOutGate);
}

void HPCC::refreshDisplay() const
{

}

void HPCC::finish()
{

}

}// namespace inet
