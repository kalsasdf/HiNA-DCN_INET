// Copyright (C)
/*
 * Developed by HDH
 * Begin at 2023/04/27
*/
#include "POSEIDON.h"

namespace inet {

Define_Module(POSEIDON);

void POSEIDON::initialize(int stage)
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
        PARA_P = par("PARA_P");
        PARA_K = par("PARA_K");
        m = par("m");
        min_md = par("min_md");

        snd_cwnd = max_cwnd = linkspeed*baseRTT.dbl()/(1538*8);EV<<"max_cwnd = "<<max_cwnd<<endl;
        max_rate = currentRate = linkspeed;
        min_rate = min_cwnd*1538*8/baseRTT.dbl();

        senddata = new TimerMsg("senddata");
        senddata->setKind(SENDDATA);

        timeout = new TimerMsg("timeout");
        timeout->setKind(TIMEOUT);

        currentRTTVector.setName("currentRTT (s)");
        targetVector.setName("target_delay (s)");
        cwndVector.setName("cwnd (num)");
        WATCH(timeout_num);
    }else if (stage == INITSTAGE_TRANSPORT_LAYER) {
        registerService(Protocol::udp, gate("upperIn"), gate("upperOut"));
        registerProtocol(Protocol::udp, gate("lowerOut"), gate("lowerIn"));
    }

}

void POSEIDON::handleMessage(cMessage *msg)
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

void POSEIDON::handleSelfMessage(cMessage *pck)
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
        case TIMEOUT:
        {
            time_out();
            break;
        }
    }
}

// Record the packet from app to transmit it to the dest
void POSEIDON::processUpperpck(Packet *pck)
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
                cancelEvent(senddata);
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

void POSEIDON::send_data()
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
    tag->setCreationtime(simTime());
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

    //insert POSEIDON header(INTHeader)
    auto content = makeShared<PSDINTHeader>();
    content->setTS(simTime());
    packet->insertAtFront(content);

    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);
    packet->setKind(0);
    packet->setTimestamp(simTime());
    auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
    addressReq->setSrcAddress(snd_info.srcAddr);
    addressReq->setDestAddress(snd_info.destAddr);

    EV<<"packet length = "<<packet->getByteLength()<<", current rate = "<<currentRate<<endl;
    EV<<"packet name "<<packet->getFullName()<<", ts = "<<simTime()<<"s"<<endl;

    sendDown(packet);
    nxtSendpacketid = packetid+1;
    cancelEvent(timeout);
    scheduleAt(simTime() + RTO, timeout);

    if(sender_packetMap.find(nxtSendpacketid)==sender_packetMap.end()){
        EV<<"packet run out, stopping"<<endl;
        SenderState = STOPPING;
    }
    else if(snd_cwnd>=1){
        if(snd_cwnd-(nxtSendpacketid-snd_una)>0){
            EV<<"snd_cwnd = "<<snd_cwnd<<", sended window - "<<packetid-snd_una<<endl;
            cancelEvent(senddata);
            scheduleAt(simTime(),senddata);
        }
        else{
            EV<<"Pausing !!!!!"<<endl;
            SenderState = PAUSING;
        }
    }
    else if(snd_cwnd<1){
        EV<<"snd_cwnd < 1, nxtSendtime = "<<simTime()+pacing_delay<<endl;
        cancelEvent(senddata);
        scheduleAt(simTime()+pacing_delay,senddata);
    }

}

void POSEIDON::processLowerpck(Packet *pck)
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

void POSEIDON::receive_data(Packet *pck)
{
    auto l3AddressInd = pck->getTag<L3AddressInd>();
    auto srcAddr = l3AddressInd->getSrcAddress();//get sourceAddress
    auto desAddr = l3AddressInd->getDestAddress();//get desAddress
    simtime_t ts = pck->getTimestamp();
    EV<<"ts = "<<ts<<"s"<<endl;

    const auto& INT_msg = pck->popAtFront<PSDINTHeader>(); // pop and obtain the INTHeader
    int curRcvNum;//current received packet Serial number
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        curRcvNum = region.getTag()->getPacketId();
    }
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
    intinfo->setTimestamp(ts);

    EV_INFO << "Sending INT from "<<desAddr<<" to "<< srcAddr <<endl;//could include more details
    EV_DETAIL<<"INT sequence is "<<curRcvNum<<endl;
    sendDown(intinfo);
    sendUp(pck);
}

void POSEIDON::receiveAck(Packet *pck)
{
    int ackid;
    simtime_t ts = pck->getTimestamp();
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        ackid = region.getTag()->getPacketId();
    }
    currentRTT = simTime()-ts;
    if((ts>t_last_decrease)){//different from SWIFT
        can_decrease = true;
    }
    cwnd_prev = snd_cwnd;

    EV<<" the ack sequence is "<<ackid<<endl;
    if(ackid>=snd_una){
        snd_una = ackid+1;
        EV<<"snd_una update to "<<snd_una<<endl;
        num_dupack = 0;
        num_acked = 0;
        retransmit_cnt = 0;
        sender_packetMap.erase(ackid);
    }else{
        num_dupack++;
        if(num_dupack==3){
            retransmit_cnt = 0;
            num_dupack = 0;
            num_acked = 0;
            if(can_decrease)
                snd_cwnd=snd_cwnd*min_md;
        }
    }

    simtime_t oldRTT_S = RTT_S;
    RTT_S = (1 - RTO_alpha) * oldRTT_S + RTO_alpha * currentRTT;
    RTT_D = (1 - RTO_beta) * RTT_D + RTO_beta * fabs(oldRTT_S - currentRTT);
    RTO = RTT_S + 4 * RTT_D;
    EV<<"RTO = "<<RTO<<endl;
    cancelEvent(timeout);
    scheduleAt(simTime() + RTO, timeout);

    num_acked++;

    const auto& INT_msg = pck->peekAtFront<PSDINTHeader>();
    double mpd = INT_msg->getMPD().dbl();


    EV<<"currate = "<<currentRate<<endl;
    double target = PARA_P * (log(max_rate) - log(currentRate)) / (log(max_rate) - log(min_rate)) + PARA_K;
    double ratio = exp((target - mpd) / PARA_P * (log(max_rate) - log(min_rate)) * m);

    ratio = math::clamp(ratio,0.4,2.5);
    EV<<"ratio = "<<ratio<<", mpd = "<<mpd<<", target = "<<target<<endl;
    if(mpd<=target){
        snd_cwnd = snd_cwnd*(1+(ratio-1)*num_acked/snd_cwnd);
    }else if(can_decrease){
        snd_cwnd = snd_cwnd*ratio;
        can_decrease = false;
    }

    snd_cwnd = math::clamp(snd_cwnd,min_cwnd,max_cwnd);
    EV<<"after changing, snd_cwnd = "<<snd_cwnd<<endl;
    cwndVector.recordWithTimestamp(simTime(), snd_cwnd);

    if(snd_cwnd < cwnd_prev)
    {
        t_last_decrease = simTime();
        EV << "t_last_decrease = " << t_last_decrease <<endl;
    }
    if(snd_cwnd < 1)
    {
        pacing_delay = currentRTT / snd_cwnd;

    }else if (snd_cwnd >=1)
    {
        pacing_delay = 0;
    }
    currentRate = snd_cwnd*1538*8/baseRTT;
    EV<<"currentRTT = "<<currentRTT<<"s, "<<"pacing_delay = "<<pacing_delay<<"s, the ACK computed rate is "<<currentRate<<endl;

    if(SenderState==PAUSING){
        SenderState=SENDING;
        scheduleAt(simTime(),senddata);
    }
    delete pck;
}

void POSEIDON::time_out()
{
    timeout_num++;
    retransmit_cnt++;
    EV<<"time out, retransmit_cnt = "<<retransmit_cnt<<endl;

    if(retransmit_cnt > RETX_RESET_THRESHOLD){
        snd_cwnd = min_cwnd;
    }else if(can_decrease)
    {
        snd_cwnd = (1 - min_md) * snd_cwnd;
        can_decrease = false;
    }
    if(snd_cwnd < 1)
    {
        pacing_delay = currentRTT / snd_cwnd;

    }else if (snd_cwnd >=1)
    {
        pacing_delay = 0;
    }
    RTO *= 2;  // 典型RTO机制，超时后时间乘2
    if(SenderState!=STOPPING){
        nxtSendpacketid=snd_una;
        cancelEvent(senddata);
        scheduleAt(simTime(),senddata);  // 超时后，立即重发未确认的第一个报文
        cancelEvent(timeout);
        scheduleAt(simTime() + RTO, timeout);
    }

}

void POSEIDON::sendDown(Packet *pck)
{
    EV << "sendDown " <<pck->getFullName()<<endl;
    send(pck,lowerOutGate);
}

void POSEIDON::sendUp(Packet *pck)
{
    EV<<"POSEIDON, oh sendup!"<<endl;
    send(pck,upperOutGate);
}

void POSEIDON::refreshDisplay() const
{

}

void POSEIDON::finish()
{
    recordScalar("timeout num", timeout_num);
}

}// namespace inet
