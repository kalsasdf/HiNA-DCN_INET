/*
 * XPASS.cc
 *
 *  Created on: 2021Äê3ÔÂ1ÈÕ
 *      Author: kalsasdf
 */

#include "inet/common/INETDefs.h"
#include "XPASS.h"


namespace inet {

Define_Module(XPASS);

void XPASS::initialize()
{
    outGate = gate("lowerOut");
    inGate = gate("lowerIn");
    upGate = gate("upperOut");
    downGate = gate("upperIn");
    // configuration
    activate = par("activate");
    max_pck_size = par("max_pck_size");
    linkspeed = par("linkspeed");
    targetratio = par("targetratio");
    initialrate = par("initialrate");
    maxrate = 0.05*linkspeed;
    currate = maxrate*initialrate;//initial sending rate
    wmax = par("wmax");
    wmin = par("wmin");
    // for ecn based control;
    rtt_beta=par("rtt_beta");
    thigh=par("thigh");
    tlow=par("tlow");
    useECN = par("useECN");
    useRTT = par("useRTT");
    minRTT=par("minRTT");
    gamma=par("gamma");
    alpha=par("alpha");
    Rai = par("Rai");
    Rhai = par("Rhai");

    registerService(Protocol::udp, gate("upperIn"), gate("upperOut"));
    registerProtocol(Protocol::udp, gate("lowerOut"), gate("lowerIn"));

    receiver_flowMap.clear();
    sender_flowMap.clear();

    frSteps_th=5;
    ByteCounter_th=10000000;
    targetecnratio=0;
    credit_size = 208;//(84-58)*8,58=20(IP)+14(EthernetMac)+8(EthernetPhy)+4(EthernetFcs)+12(interframe gap,IFG)
    max_idletime = 0.00002;

    sendcredit->setKind(SENDCRED);
}

void XPASS::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        handleSelfMessage(msg);// sendcredit
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

void XPASS::handleSelfMessage(cMessage *pck)
{
    TimerMsg *timer = check_and_cast<TimerMsg *>(pck);
    // Process different self-messages (timer signals)
    EV_TRACE << "Self-message " << timer << " received\n";

    switch (timer->getKind()) {
        case SENDCRED:
            send_credit(timer->getDestAddr());
            break;
        case STOPCRED:
            send_stop(timer->getDestAddr());
            break;
        case ALPHATIMER:
            updateAlpha(timer->getDestAddr());
            break;
        case RATETIMER:
        {
            receiver_flowinfo tinfo = receiver_flowMap.find(timer->getDestAddr())->second;
            tinfo.TimeFrSteps++;
            receiver_flowMap[timer->getDestAddr()] = tinfo;
            increaseTxRate(timer->getDestAddr());
            cancelEvent(tinfo.rateTimer);
            scheduleAt(simTime()+nowRTT+0.000005,tinfo.rateTimer);
            break;
        }
        case SENDSTOP:
            send_stop(timer->getDestAddr());
            break;
    }
}

void XPASS::processUpperpck(Packet *pck)
{
    if (string(pck->getFullName()).find("Data") != string::npos&&activate==true)
    {
        auto addressReq = pck->addTagIfAbsent<L3AddressReq>();
        auto udpHeader = pck->peekAtFront<UdpHeader>();
        srcaddr = addressReq->getSrcAddress();
        L3Address destAddr = addressReq->getDestAddress();
        if(sender_StateMap.find(destAddr)==sender_StateMap.end()){
            sender_StateMap[destAddr] = CSTOP_SENT;
        }

        sender_flowinfo snd_info;
        for (auto& region : pck->peekData()->getAllTags<HiTag>()){
            snd_info.flowid = region.getTag()->getFlowId();
            snd_info.flowsize = region.getTag()->getFlowSize();
            snd_info.cretime = region.getTag()->getCreationtime();
            EV << "store new flow, id = "<<snd_info.flowid<<", size = "<<snd_info.flowsize<<
                    ", creationtime = "<<snd_info.cretime<<endl;
        }
        remainSize += snd_info.flowsize;

        snd_info.destaddr = destAddr;
        snd_info.srcPort = udpHeader->getSrcPort();
        snd_info.destPort = udpHeader->getDestPort();
        snd_info.crcMode = udpHeader->getCrcMode();
        snd_info.crc = udpHeader->getCrc();
        sender_flowMap[snd_info.flowid] = snd_info;

        if(sender_StateMap[destAddr]==CSTOP_SENT)
            send_credreq(destAddr);
        delete pck;
    }
    else
    {
        EV<<"Unknown packet, sendDown()."<<endl;
        sendDown(pck);
    }
}

void XPASS::processLowerpck(Packet *pck)
{
    if (!strcmp(pck->getFullName(),"credit_req"))
    {
        receive_credreq(pck);
    }
    else if (!strcmp(pck->getFullName(),"credit"))
    {
        receive_credit(pck);
    }
    else if (!strcmp(pck->getFullName(),"credit_stop"))
    {
        receive_stopcred(pck);
    }
    else if (string(pck->getFullName()).find(packetName) != string::npos)
    {
        receive_data(pck);
    }
    else
    {
        sendUp(pck);
    }
}

void XPASS::send_credreq(L3Address destaddr)
{
    sender_StateMap[destaddr] = CREQ_SENT;
    Packet *cred_req = new Packet("credit_req");
    cred_req->addTagIfAbsent<L3AddressReq>()->setDestAddress(destaddr);
    cred_req->addTagIfAbsent<L3AddressReq>()->setSrcAddress(srcaddr);
    cred_req->setTimestamp(simTime());

    cred_req->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    cred_req->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

    EV << "send creditreq to "<<destaddr.toIpv4()<<", dest state = "<<sender_StateMap[destaddr]<<endl;
    sendDown(cred_req); // send down the credit_request.
}

// Receive the SYN packet from the sender, and begin sending credit.
void XPASS::receive_credreq(Packet *pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    srcaddr = l3addr->getDestAddress();
    if(receiver_StateMap.find(l3addr->getSrcAddress())==receiver_StateMap.end()){
        receiver_StateMap[l3addr->getSrcAddress()]=CREDIT_STOP;
    }

    if(receiver_StateMap[l3addr->getSrcAddress()]==CREDIT_SENDING){
        delete pck;
    }else{
        receiver_flowinfo rcvflow;
        if(receiver_flowMap.find(l3addr->getSrcAddress())==receiver_flowMap.end()){
            rcvflow.destaddr=l3addr->getSrcAddress();
            rcvflow.last_Fbtime = simTime();
            rcvflow.nowRTT = 2*(simTime() - pck->getTimestamp());
            rcvflow.pck_in_rtt = 0;
            rcvflow.last_received_data_seq = -1;
            rcvflow.now_received_data_seq = 0;
            rcvflow.now_send_cdt_seq = 0;
            rcvflow.sumlost = 0;
            rcvflow.current_speed = currate;
            rcvflow.targetRate=maxrate;
            rcvflow.max_speed = maxrate;

            //for ECN-based rate control
            rcvflow.omega =1/16;
            rcvflow.alpha=alpha;
            rcvflow.ByteFrSteps=0;
            rcvflow.ByteCounter=0;
            rcvflow.TimeFrSteps=0;
            rcvflow.ecn_in_rtt = 0;
            rcvflow.iRhai=0;
            rcvflow.previousincrease = false;
            rcvflow.ReceiverState=Normal;
            rcvflow.rateTimer = new TimerMsg("rateTimer");
            rcvflow.rateTimer->setKind(RATETIMER);
            rcvflow.rateTimer->setDestAddr(l3addr->getSrcAddress());
            rcvflow.alphaTimer = new TimerMsg("alphaTimer");
            rcvflow.alphaTimer->setKind(ALPHATIMER);
            rcvflow.alphaTimer->setDestAddr(l3addr->getSrcAddress());

            receiver_flowMap[l3addr->getSrcAddress()]=rcvflow;
        }else{
            rcvflow = receiver_flowMap[l3addr->getSrcAddress()];
        }
        EV<<"receive credreq, nowRTT = "<<rcvflow.nowRTT<<endl;
        sendcredit->setDestAddr(l3addr->getSrcAddress());
        scheduleAt(simTime(),sendcredit);
        receiver_StateMap[l3addr->getSrcAddress()]=CREDIT_SENDING;
        delete pck;
    }
}

// Generate and send credit to the destination.
void XPASS::send_credit(L3Address destaddr)
{
    Packet *credit = new Packet("credit");

    receiver_flowinfo rcvflow = receiver_flowMap[destaddr];
    int jitter_bytes = intrand(8)+1;
    const auto& content = makeShared<ByteCountChunk>(B(credit_size/8+jitter_bytes));
    auto tag = content->addTag<HiTag>();
    tag->setPacketId(rcvflow.now_send_cdt_seq);

    credit->insertAtFront(content);

    credit->setTimestamp(simTime());
    credit->addTagIfAbsent<L3AddressReq>()->setDestAddress(rcvflow.destaddr);
    credit->addTagIfAbsent<L3AddressReq>()->setSrcAddress(srcaddr);
    credit->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    credit->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

    send(credit, outGate);
    EV<<"send credit "<<rcvflow.now_send_cdt_seq<<endl;
    rcvflow.now_send_cdt_seq++;
    receiver_flowMap[destaddr] = rcvflow;
    if(receiver_StateMap[destaddr]==CREDIT_SENDING){
        sendcredit->setDestAddr(destaddr);
        scheduleAt(simTime() + (simtime_t)((credit_size+(jitter_bytes+58)*8)/rcvflow.current_speed),sendcredit);
        EV<<"currate = "<<rcvflow.current_speed<<", next time = "<<simTime() + (credit_size+jitter_bytes*8)/rcvflow.current_speed<<"s"<<endl;
    }
}

// Receive the credit, and send corresponding TCP segment.
void XPASS::receive_credit(Packet *pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();

    EV<<"receive_credit(), the source "<<l3addr->getSrcAddress()<<endl;

    long packetid;
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        packetid = region.getTag()->getPacketId();
        EV << "credit_id = "<<packetid<<endl;
    }

    // if no flow exits to be transmitted, stop transmitting.
    if (remainSize == 0 && sender_StateMap[l3addr->getSrcAddress()]==CREDIT_RECEIVING)
    {
        // send stop credit to the receiver
        TimerMsg *sendstop = new TimerMsg("sendstop");
        sendstop->setKind(SENDSTOP);
        sendstop->setDestAddr(l3addr->getSrcAddress());
        EV<<"receive_credit(), remainsize = 0, max_idletime = "<<max_idletime<<endl;
        scheduleAt(simTime() + max_idletime,sendstop);
    }
    else
    {
        // none stop
        sender_flowinfo sndflow= sender_flowMap.begin()->second;
        int flowid =sender_flowMap.begin()->first;
        std::ostringstream str;
        str << packetName << "-" <<flowid<<"-"<<packetid;
        Packet *packet = new Packet(str.str().c_str());
        int packetLength;
        if (sndflow.flowsize >= max_pck_size)
        {
            remainSize -=  max_pck_size;
            sender_flowMap[flowid].flowsize-=  max_pck_size;
            packetLength=max_pck_size;
        }
        else if(remainSize>=max_pck_size)
        {
            sender_flowMap.erase(flowid);
            sender_flowMap.begin()->second.flowsize-=sndflow.flowsize;
            remainSize -= max_pck_size;
            packetLength=max_pck_size;
        }
        else{
            remainSize=0;
            packetLength=remainSize;
        }
        const auto& payload = makeShared<ByteCountChunk>(B(packetLength));
        auto tag = payload->addTag<HiTag>();
        tag->setReverse(true);
        tag->setFlowId(flowid);
        tag->setPacketId(packetid);
        tag->setCreationtime(sndflow.cretime);

        packet->insertAtBack(payload);

        auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
        addressReq->setSrcAddress(l3addr->getDestAddress());
        addressReq->setDestAddress(l3addr->getSrcAddress());

        // insert udpHeader, set source and destination port
        const Protocol *l3Protocol = &Protocol::ipv4;
        auto udpHeader = makeShared<UdpHeader>();
        udpHeader->setSourcePort(sndflow.srcPort);
        udpHeader->setDestinationPort(sndflow.destPort);
        udpHeader->setCrc(sndflow.crc);
        udpHeader->setCrcMode(sndflow.crcMode);
        udpHeader->setTotalLengthField(udpHeader->getChunkLength() + packet->getTotalLength());
        insertTransportProtocolHeader(packet, Protocol::udp, udpHeader);
        packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);
        if(useECN)
            packet->addTagIfAbsent<EcnReq>()->setExplicitCongestionNotification(1);
        packet->setKind(0);
        packet->setTimestamp(pck->getTimestamp());

        EV << "prepare to send packet, remaining data size = " << remainSize <<endl;
        sendDown(packet);
    }
    delete pck;
}

// Receive the TCP segment, determine the credit loss ratio and enter the feedback control.
void XPASS::receive_data(Packet *pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();

    auto ecn = pck->addTagIfAbsent<EcnInd>()->getExplicitCongestionNotification();
    auto it = receiver_flowMap.find(l3addr->getSrcAddress());

    if (it!=receiver_flowMap.end())
    {
        if (useECN) // use ecn for rate control
        {
            receiver_flowinfo tinfo = it->second;
            if(tinfo.ReceiverState!=Normal)
            {
                tinfo.ByteCounter+=pck->getByteLength();
                EV<<"receive_data(), bytecounter = "<<tinfo.ByteCounter;
                if(tinfo.ByteCounter>=ByteCounter_th)
                {
                    tinfo.ByteFrSteps++;
                    tinfo.ByteCounter-=ByteCounter_th;
                    EV<<"byte counter expired, byte fr steps = "<<tinfo.ByteFrSteps<<endl;
                    increaseTxRate(l3addr->getSrcAddress());
                }
                else{
                    receiver_flowMap[l3addr->getSrcAddress()]=tinfo;
                }
            }
            EV<<"receive_data(), now the RTT = "<<tinfo.nowRTT<<"s, the time gap from last ECN-based rate control = "<< simTime()-tinfo.last_Fbtime <<"s"<<endl;
            tinfo.pck_in_rtt++;

            // Calculate the credit loss ratio, and enter the feedback control.
            if ((simTime()-tinfo.last_Fbtime) < tinfo.nowRTT )
            {
                if (ecn == 3)
                {
                    tinfo.ecn_in_rtt++;

                }
            }
            else
            {
                tinfo.preRTT=tinfo.nowRTT;
                tinfo.nowRTT = simTime() - pck->getTimestamp();
                nowRTT=tinfo.nowRTT;
                tinfo.sumlost = tinfo.now_received_data_seq - tinfo.last_received_data_seq - tinfo.pck_in_rtt;
                if (ecn == 3)
                {
                    tinfo.ecn_in_rtt++;

                }
                EV<<"receive_data(), ecn_in_rtt = "<<tinfo.ecn_in_rtt<<", pck_in_rtt = "<<tinfo.pck_in_rtt<<endl;
                EV<<"preRTT="<<tinfo.preRTT<<",nowRTT="<<tinfo.nowRTT<<",rtt_diff="<<tinfo.rtt_diff<<",minRTT="<<minRTT<<endl;
                tinfo = ecnbased_control(tinfo);
                tinfo.last_received_data_seq = tinfo.now_received_data_seq;
                tinfo.last_Fbtime = simTime();
                tinfo.pck_in_rtt = 0;
                tinfo.ecn_in_rtt = 0;
            }
            receiver_flowMap[l3addr->getSrcAddress()] = tinfo;

            sendUp(pck);
        }
        else //// use lossratio for feedback control
        {
            receiver_flowinfo tinfo = it->second;
            for (auto& region : pck->peekData()->getAllTags<HiTag>()){
                tinfo.now_received_data_seq = region.getTag()->getPacketId();
            }
            EV<<"receive_data(), nowRTT = "<<tinfo.nowRTT<<"s, the time gap from last feedback control = "<< simTime()-tinfo.last_Fbtime <<"s"<<endl;
            // Calculate the credit loss ratio, and enter the feedback control.
            if ((simTime()-tinfo.last_Fbtime) < tinfo.nowRTT)
            {
                tinfo.pck_in_rtt++;
            }
            else
            {
                tinfo.pck_in_rtt++;
                tinfo.sumlost = tinfo.now_received_data_seq - tinfo.last_received_data_seq - tinfo.pck_in_rtt;
                EV<<"receive_data(), sumlost = "<<tinfo.sumlost<<", now_received_cdt_seq = "<<tinfo.now_received_data_seq<<", last_received_cdt_seq = "<<tinfo.last_received_data_seq<<", pck_in_rtt = "<<tinfo.pck_in_rtt<<endl;
                tinfo = feedback_control(tinfo);
                tinfo.nowRTT = simTime() - pck->getTimestamp();
                tinfo.last_Fbtime = simTime();
                tinfo.sumlost = 0;
                tinfo.last_received_data_seq = tinfo.now_received_data_seq;
                tinfo.pck_in_rtt = 0;
            }
            receiver_flowMap[l3addr->getSrcAddress()] = tinfo;
            sendUp(pck);
        }
    }
    else // packet dont controlled by credits.
    {
        sendUp(pck);
    }
}

// Generate and send stop credit Packet to the destination.
void XPASS::send_stop(L3Address destaddr)
{
    sender_StateMap[destaddr] = CSTOP_SENT;
    Packet *cred_stop = new Packet("credit_stop");
    cred_stop->addTagIfAbsent<L3AddressReq>()->setDestAddress(destaddr);
    cred_stop->addTagIfAbsent<L3AddressReq>()->setSrcAddress(srcaddr);
    cred_stop->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    cred_stop->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

    EV << "send_stop(), DestAddr = "<<destaddr.toIpv4()<<endl;
    send(cred_stop, outGate);// send down the credit_stop.
}

// Receive the stop credit Packet from sender, and stop sending credit.
void XPASS::receive_stopcred(Packet *pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    EV<<"stop credit sending for "<<l3addr->getSrcAddress()<<endl;
    receiver_StateMap[l3addr->getSrcAddress()]=CREDIT_STOP;

    delete pck;
}

void XPASS::increaseTxRate(L3Address destaddr)
{
    receiver_flowinfo tinfo = receiver_flowMap.find(destaddr)->second;
    double oldspeed = tinfo.current_speed;
    EV<<"entering timer."<<endl;
    EV<<"oldspeed = "<<oldspeed<<endl;

    if (max(tinfo.ByteFrSteps,tinfo.TimeFrSteps) < frSteps_th)
    {
        tinfo.ReceiverState = Fast_Recovery;
    }
    else if (min(tinfo.ByteFrSteps,tinfo.TimeFrSteps) > frSteps_th)
    {
        tinfo.ReceiverState = Hyper_Increase;
    }
    else
    {
        tinfo.ReceiverState = Additive_Increase;
    }


    if (tinfo.ReceiverState == Fast_Recovery)
    {// Fast Recovery
        EV<<"enter Fast_Recovery,targetRate="<<tinfo.targetRate<<endl;
        tinfo.current_speed = (tinfo.current_speed + tinfo.targetRate) / 2;
    }
    else if (tinfo.ReceiverState == Additive_Increase)
    {// Additive Increase
        EV<<"enter Additive_Increase,targetRate="<<tinfo.targetRate<<", Rai = "<<Rai<<endl;
        tinfo.targetRate += Rai;
        EV<<"after increasing targetRate="<<tinfo.targetRate<<endl;
        tinfo.targetRate = (tinfo.targetRate > maxrate) ? maxrate : tinfo.targetRate;
        tinfo.current_speed = (tinfo.current_speed + tinfo.targetRate) / 2;
    }
    else if (tinfo.ReceiverState == Hyper_Increase)
    {// Hyper Increase
        EV<<"enter Hyper_Increase,targetRate="<<tinfo.targetRate<<endl;
        tinfo.iRhai++;
        tinfo.targetRate += tinfo.iRhai*Rhai;
        EV<<"after increasing targetRate="<<tinfo.targetRate<<endl;
        tinfo.targetRate = (tinfo.targetRate > maxrate) ? maxrate : tinfo.targetRate;
        tinfo.current_speed = (tinfo.current_speed + tinfo.targetRate) / 2;
    }
    else
    {// Normal state

    }
    EV<<"after increasing,oldspeed="<<oldspeed<<",newspeed="<<tinfo.current_speed<<endl;
    receiver_flowMap[destaddr] = tinfo;
}

void XPASS::updateAlpha(L3Address destaddr)
{
    receiver_flowinfo tinfo = receiver_flowMap.find(destaddr)->second;
    tinfo.alpha = (1 - gamma) * tinfo.alpha;
    scheduleAt(simTime() + nowRTT+0.000005, tinfo.alphaTimer);
    receiver_flowMap[destaddr] = tinfo;
}

void XPASS::sendDown(Packet *pck)
{
    send(pck, outGate);
}

void XPASS::sendUp(Packet *pck)
{
    EV<<"XPASS, oh sendup!"<<endl;
    send(pck,upGate);

}

void XPASS::finish()
{

}
} // namespace inet


