/*
 * SWIFT.cc
 *
 *  Created on: 20230205
 *      Author: luca
 */
#include "SWIFT.h"

namespace inet {

Define_Module(SWIFT);

void SWIFT::initialize(){
    //gates
    lowerOutGate = gate("lowerOut");
    lowerInGate = gate("lowerIn");
    upperOutGate = gate("upperOut");
    upperInGate = gate("upperIn");
    // configuration
    stopTime = par("stopTime");
    activate = par("activate");
    linkspeed = par("linkspeed");
    max_pck_size = par("max_pck_size");
    baseRTT = par("baseRTT");
    hop_scaler = par("hop_scaler");
    hops = par("hops");
    ai = par("ai");
    beta = par("beta");
    max_mdf = par("max_mdf");
    fs_min_cwnd = par("fs_min_cwnd");
    fs_max_cwnd = par("fs_max_cwnd");

    snd_cwnd = send_window = max_cwnd = linkspeed*baseRTT.dbl()/(max_pck_size*8);
    min_cwnd = 0.001;
    fs_range = 4 * baseRTT.dbl();

    alpha = fs_range/((1.0/sqrt(fs_min_cwnd)) - (1.0/sqrt(fs_max_cwnd)));

    senddata = new TimerMsg("senddata");
    senddata->setKind(SENDDATA);

    timeout = new TimerMsg("timeout");
    timeout->setKind(TIMEOUT);

    currentRTTVector.setName("currentRTT (s)");
    targetVector.setName("target_delay (s)");
    cwndVector.setName("cwnd (num)");

    registerService(Protocol::udp, gate("upperIn"), gate("upperOut"));
    registerProtocol(Protocol::udp, gate("lowerOut"), gate("lowerIn"));
}

void SWIFT::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        handleSelfMessage(msg);
    else
    {
        if (msg->arrivedOn("upperIn"))
        {
            if (string(msg->getFullName()).find("Data") != string::npos)
                processUpperpck(check_and_cast<Packet*>(msg));
            else
                send(msg,lowerOutGate);
        }
        else if (msg->arrivedOn("lowerIn"))
        {
            processLowerpck(check_and_cast<Packet*>(msg));
        }
    }
}

void SWIFT::handleSelfMessage(cMessage *pck)
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

void SWIFT::processUpperpck(Packet *pck)
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
            }else{
                snd_info.length = max_pck_size;
            }

            if(sender_packetMap.empty()){
                sender_packetMap[packetid]=snd_info;
                iter=sender_packetMap.begin();//iter needs to be assigned after snd_info is inserted
            }else{
                sender_packetMap[packetid]=snd_info;
            }
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

void SWIFT::send_data()
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

    packet->insertAtBack(payload);

    auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
    addressReq->setSrcAddress(snd_info.srcAddr);
    addressReq->setDestAddress(snd_info.destAddr);

    //generate and insert a new udpHeader, set source and destination port
    const Protocol *l3Protocol = &Protocol::ipv4;
    auto udpHeader = makeShared<UdpHeader>();
    udpHeader->setSourcePort(snd_info.srcPort);
    udpHeader->setDestinationPort(snd_info.destPort);
    udpHeader->setCrc(snd_info.crc);
    udpHeader->setCrcMode(snd_info.crcMode);
    udpHeader->setTotalLengthField(udpHeader->getChunkLength()+packet->getTotalLength());
    insertTransportProtocolHeader(packet, Protocol::udp, udpHeader);
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);
    packet->setKind(0);
    packet->setTimestamp(simTime());

    EV<<"packet length = "<<packet->getByteLength()<<endl;

    sendDown(packet);

    if(FAST_RECOVERY){
        if(!sacks_array_snd.empty())
        {
            sacks_array_snd.pop_front();
            nxtSendpacketid = sacks_array_snd.front();
            if(sacks_array_snd.empty())
            {
                FAST_RECOVERY=false;
                EV << "SACK array is empty" <<endl;
                nxtSendpacketid = pre_snd;
            }
        }
    }else{
        nxtSendpacketid = packetid+1;
    }

    if(sender_packetMap.find(nxtSendpacketid)==sender_packetMap.end()){
        EV<<"packet run out, stopping"<<endl;
        SenderState = STOPPING;
    }
    else if(snd_cwnd>=1){
        if(snd_cwnd-(nxtSendpacketid-snd_una)>0){
            scheduleAt(simTime(),senddata);
        }
        else{
            EV<<"Pausing !!!!!"<<endl;
            SenderState = PAUSING;
        }
    }
    else if(snd_cwnd<1){
        cancelEvent(senddata);
        scheduleAt(simTime()+pacing_delay,senddata);
    }

}

// Record the packet from udp to transmit it to the dest


void SWIFT::processLowerpck(Packet *pck)
{
    if (string(pck->getFullName()).find(packetName) != string::npos)
    {
        receive_data(pck);
    }
    else if(string(pck->getFullName()).find("ACK") != string::npos)
    {
        receive_ack(pck);
    }
    else
    {
        sendUp(pck);
    }
}

void SWIFT::receive_data(Packet *pck)
{
    auto l3AddressInd = pck->getTag<L3AddressInd>();
    auto srcAddr = l3AddressInd->getSrcAddress();//get sourceAddress
    auto desAddr = l3AddressInd->getDestAddress();//get desAddress
    simtime_t ts = pck->getTimestamp();

    int curRcvNum;//current received packet Serial number
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        curRcvNum = region.getTag()->getPacketId();
    }
    EV_DETAIL<<"sequence is "<<curRcvNum<<endl;

    if(receiver_Map.find(srcAddr)==receiver_Map.end()){
        if(curRcvNum==receiver_Map[srcAddr].rcv_nxt){
            receiver_Map[srcAddr].rcv_nxt++;
            EV_DETAIL<<"packet is ordered, send ACK!"<<endl;
            std::ostringstream str;
            str <<"ACK-" <<curRcvNum;
            Packet *ack = new Packet(str.str().c_str());
            const auto& payload = makeShared<ByteCountChunk>(B(1));
            auto tag=payload->addTag<HiTag>();
            tag->setPacketId(curRcvNum);
            ack->insertAtBack(payload);

            auto addressReq = ack->addTagIfAbsent<L3AddressReq>();
            addressReq->setSrcAddress(desAddr);
            addressReq->setDestAddress(srcAddr);
            ack->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
            ack->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);
            ack->setTimestamp(ts);

            sendDown(ack);
            sendUp(pck);
        }
        else{
            SackItem *sackItem = new SackItem;
            sackItem->setPacketid(curRcvNum);
            receiver_Map[srcAddr].sacks_array.push_front(*sackItem);

            EV_DETAIL<<"packet is out of order, send SACK!"<<endl;
            std::ostringstream str;
            str <<"SACK-" <<receiver_Map[srcAddr].rcv_nxt;
            Packet *sack = new Packet(str.str().c_str());
            const auto& payload = makeShared<ByteCountChunk>(B(1));
            auto tag=payload->addTag<HiTag>();
            tag->setPacketId(receiver_Map[srcAddr].rcv_nxt);
            sack->insertAtBack(payload);

            auto content = makeShared<SACK>();

            uint counter = 0;
            uint n = receiver_Map[srcAddr].sacks_array.size();
            if(n>4)
                n=4;//maxnode
            content->setSackItemArraySize(n);
            for (auto it = receiver_Map[srcAddr].sacks_array.begin(); it != receiver_Map[srcAddr].sacks_array.end() && counter < n; it++) {
                content->setSackItem(counter++, *it);
            }

            sack->insertAtFront(content);

            auto addressReq = sack->addTagIfAbsent<L3AddressReq>();
            addressReq->setSrcAddress(desAddr);
            addressReq->setDestAddress(srcAddr);
            sack->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
            sack->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);
            sack->setTimestamp(ts);

            sendDown(sack);
            delete pck;
        }
    }
    else{
        if(curRcvNum == receiver_Map[srcAddr].rcv_nxt)//ordered
        {
            receiver_Map[srcAddr].rcv_nxt++;

            while(find(receiver_Map[srcAddr].acksequence.begin(),receiver_Map[srcAddr].acksequence.end(),receiver_Map[srcAddr].rcv_nxt)!=receiver_Map[srcAddr].acksequence.end())
            {
                receiver_Map[srcAddr].acksequence.erase(find(receiver_Map[srcAddr].acksequence.begin(),receiver_Map[srcAddr].acksequence.end(),receiver_Map[srcAddr].rcv_nxt));
                receiver_Map[srcAddr].rcv_nxt++;
            }

            EV_DETAIL<<"packet is ordered, send ACK-"<<curRcvNum<<endl;
            std::ostringstream str;
            str <<"ACK-" <<curRcvNum;
            Packet *ack = new Packet(str.str().c_str());
            const auto& payload = makeShared<ByteCountChunk>(B(1));
            auto tag=payload->addTag<HiTag>();
            tag->setPacketId(curRcvNum);
            ack->insertAtBack(payload);

            auto addressReq = ack->addTagIfAbsent<L3AddressReq>();
            addressReq->setSrcAddress(desAddr);
            addressReq->setDestAddress(srcAddr);
            ack->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
            ack->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);
            ack->setTimestamp(ts);

            sendDown(ack);
            sendUp(pck);
        }
        else if(curRcvNum > receiver_Map[srcAddr].rcv_nxt)//out of order
        {
            receiver_Map[srcAddr].acksequence.push_back(curRcvNum);

            auto it = receiver_Map[srcAddr].sacks_array.begin();
            while (it != receiver_Map[srcAddr].sacks_array.end()) {
                if (it->getPacketid()<receiver_Map[srcAddr].rcv_nxt) {
                    EV_DETAIL << "\t SACK in sacks_array: " << " " << it->str() << " delete now\n";
                    it = receiver_Map[srcAddr].sacks_array.erase(it);
                }
                else {
                    EV_DETAIL << "\t SACK in sacks_array: " << " " << it->str() << endl;
                    it++;
                }
            }
            SackItem *sackItem = new SackItem;
            sackItem->setPacketid(curRcvNum);
            receiver_Map[srcAddr].sacks_array.push_front(*sackItem);

            it = receiver_Map[srcAddr].sacks_array.begin();
            for (; it != receiver_Map[srcAddr].sacks_array.end(); it++) {
                auto it2 = it;
                it2++;
                while (it2 != receiver_Map[srcAddr].sacks_array.end()) {
                    if (it->getPacketid()==it2->getPacketid()) {
                        EV_DETAIL << "sack matched, delete contained : a=" << it->str() << ", b=" << it2->str() << endl;
                        it2 = receiver_Map[srcAddr].sacks_array.erase(it2);
                    }
                    else
                        it2++;
                }
            }

            EV_DETAIL<<"packet is out of order, send SACK!"<<endl;
            std::ostringstream str;
            str <<"SACK-" <<receiver_Map[srcAddr].rcv_nxt;
            Packet *sack = new Packet(str.str().c_str());
            const auto& payload = makeShared<ByteCountChunk>(B(1));
            auto tag=payload->addTag<HiTag>();
            tag->setPacketId(receiver_Map[srcAddr].rcv_nxt);
            sack->insertAtBack(payload);

            auto content = makeShared<SACK>();
            uint counter = 0;
            uint n = receiver_Map[srcAddr].sacks_array.size();
            if(n>4)
                n=4;//maxnode
            content->setSackItemArraySize(n);
            for (it = receiver_Map[srcAddr].sacks_array.begin(); it != receiver_Map[srcAddr].sacks_array.end() && counter < n; it++) {
                content->setSackItem(counter++, *it);
            }

            sack->insertAtFront(content);

            auto addressReq = sack->addTagIfAbsent<L3AddressReq>();
            addressReq->setSrcAddress(desAddr);
            addressReq->setDestAddress(srcAddr);
            sack->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
            sack->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);
            sack->setTimestamp(ts);

            sendDown(sack);
            delete pck;
        }
        else{
            sendUp(pck);
        }
    }
}

void SWIFT::receive_ack(Packet *pck)
{
    int ackid;
    simtime_t ts = pck->getTimestamp();
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        ackid = region.getTag()->getPacketId();
    }
    currentRTT = simTime()-ts;
    EV<<"ts = "<<ts<<", t_last_decrease = "<<t_last_decrease<<endl;
    if((ts>=t_last_decrease)){
        can_decrease = true;
    }

    cwnd_prev = snd_cwnd;

    EV<<"ackid = "<<ackid<<", snd_una = "<<snd_una<<endl;
    if(ackid>=snd_una){
        snd_una = ackid+1;
        EV<<"snd_una update to "<<snd_una<<endl;
        num_dupack = 0;
        num_acked = 0;
        retransmit_cnt = 0;
    }else if(string(pck->getFullName()).find("SACK") != string::npos){
        auto SACK_msg = pck->peekAtFront<SACK>();
        uint n = SACK_msg->getSackItemArraySize();
        if(n>0){
            num_dupack++;
            for(uint i=0; i<n;i++){
                for(uint i2=ackid+1;i2<SACK_msg->getSackItem(i).getPacketid();i2++){
                    if(std::find(sacks_array_snd.begin(),sacks_array_snd.end(),i2)==sacks_array_snd.end())
                        sacks_array_snd.push_back(i2);
                }
            }
        }
        if(num_dupack==3){
            retransmit_cnt = 0;
            num_dupack = 0;
            num_acked = 0;
            FAST_RECOVERY=true;
            pre_snd = nxtSendpacketid;
            nxtSendpacketid = sacks_array_snd.front();
            send_data();
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

    double flow_scaling = std::min((alpha/sqrt(snd_cwnd)) - alpha/sqrt(fs_max_cwnd), fs_range);
    flow_scaling = std::max(0.0, flow_scaling);
    EV<<"flow_scaling = "<<flow_scaling<<endl;
    simtime_t target_delay = baseRTT + hops * hop_scaler + flow_scaling;

    EV <<"currentRTT = "<<currentRTT<<", target delay = "<< target_delay << ", before changing cwnd = " << snd_cwnd << ", can_decrease is " << can_decrease <<endl;
    currentRTTVector.recordWithTimestamp(simTime(), currentRTT);
    targetVector.recordWithTimestamp(simTime(), target_delay);

    double temp_cwnd = snd_cwnd;

    if(currentRTT < target_delay)
    {
        if(temp_cwnd >=1){
            temp_cwnd += ai / temp_cwnd * num_acked;
        }
        else{
            temp_cwnd += ai * num_acked;
        }

    }else{
        if(can_decrease){  // beta 0.8  max_mdf = 0.5  ai没有
            temp_cwnd = max(1 - beta*(currentRTT-target_delay)/(currentRTT), 1-max_mdf) * temp_cwnd;
            can_decrease = false;
        }
    }

    snd_cwnd = temp_cwnd;
    cwndVector.recordWithTimestamp(simTime(), snd_cwnd);

    snd_cwnd = snd_cwnd > max_cwnd? max_cwnd : snd_cwnd;
    snd_cwnd = snd_cwnd > min_cwnd? snd_cwnd : min_cwnd;
    EV<<"after changing cwnd is "<<snd_cwnd<<endl;

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
//    send_window=snd_cwnd;
    if(SenderState==PAUSING){
        SenderState=SENDING;
        scheduleAt(simTime(),senddata);
    }
    delete pck;
}

void SWIFT::time_out()
{
    timeout_num++;

    retransmit_cnt++;

    if(retransmit_cnt > RETX_RESET_THRESHOLD)
        snd_cwnd = min_cwnd;

    if(can_decrease)
    {
        snd_cwnd = (1 - max_mdf) * snd_cwnd;
        can_decrease = false;
    }
    RTO *= 2;  // 典型RTO机制，超时后时间乘2
    send_window=snd_cwnd;
    nxtSendpacketid=snd_una;
    send_data();  // 超时后，立即重发未确认的第一个报文
    cancelEvent(timeout);
    scheduleAt(simTime() + RTO, timeout);
}

void SWIFT::sendDown(Packet *pck)
{
    EV << "SWIFT, sendDown " <<pck->getFullName()<<endl;
    send(pck,lowerOutGate);
}

void SWIFT::sendUp(Packet *pck)
{
    EV<<"SWIFT, sendup!"<<endl;
    send(pck,upperOutGate);
}


void SWIFT::refreshDisplay() const
{

}


void SWIFT::finish()
{
    recordScalar("timeout num", timeout_num);
}

}
