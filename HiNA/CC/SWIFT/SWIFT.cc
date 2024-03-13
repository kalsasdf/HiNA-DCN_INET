/*
 * SWIFT.cc
 *
 *  Created on: 20230205
 *      Author: HiNA
 */
#include "SWIFT.h"

namespace inet {

Define_Module(SWIFT);

void SWIFT::initialize(int stage)
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
        max_pck_size = par("max_pck_size");
        maxInterval = par("maxInterval");
        baseRTT = par("baseRTT");
        hop_scaler = par("hop_scaler");
        hops = par("hops");
        ai = par("ai");
        beta = par("beta");
        max_mdf = par("max_mdf");
        fs_min_cwnd = par("fs_min_cwnd");
        fs_max_cwnd = par("fs_max_cwnd");

        snd_cwnd = max_cwnd = linkspeed*baseRTT.dbl()/(1526*8);
        fs_range = 4 * baseRTT.dbl();

        alpha = fs_range/((1.0/sqrt(fs_min_cwnd)) - (1.0/sqrt(fs_max_cwnd)));

        senddata = new TimerMsg("senddata");
        senddata->setKind(SENDDATA);

        timeout = new TimerMsg("timeout");
        timeout->setKind(TIMEOUT);

        currentRTTVector.setName("currentRTT (s)");
        targetVector.setName("target_delay (s)");
        cwndVector.setName("cwnd (num)");
        currateVector.setName("currate");
        WATCH(timeout_num);
    }else if (stage == INITSTAGE_TRANSPORT_LAYER) {
        registerService(Protocol::udp, gate("upperIn"), gate("upperOut"));
        registerProtocol(Protocol::udp, gate("lowerOut"), gate("lowerIn"));
    }
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

void SWIFT::send_data()
{
    int packetid = nxtSendpacketid;
    sender_packetinfo snd_info = sender_packetMap.find(packetid)->second;
    if(sender_packetMap.find(packetid)==sender_packetMap.end())
        throw cRuntimeError("packet doesn't exist");
    EV<<"send_data(), packetid = "<<packetid<<", prepare to send packet to destination "<<snd_info.destAddr.toIpv4()<<endl;
    std::ostringstream str;
    str << packetName << "-" <<packetid;
    Packet *packet = new Packet(str.str().c_str());
    const auto& payload = makeShared<ByteCountChunk>(B(snd_info.length));
    auto tag = payload->addTag<HiTag>();
    tag->setFlowId(snd_info.flowid);
    tag->setPriority(snd_info.priority);
    if(lastflowid!=snd_info.flowid){
        last_creation_time = snd_info.cretime;
        lastflowid = snd_info.flowid;
    }
    tag->setCreationtime(last_creation_time);
    tag->setPacketId(packetid);
    tag->setIsLastPck(snd_info.last);

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

    bitlength+=(snd_info.length+58)*8;
    //58=20(IP)+14(EthernetMac)+8(EthernetPhy)+4(EthernetFcs)+12(interframe gap,IFG)
    if(simTime()-lasttime>=maxInterval){
        currentRate = bitlength/(simTime()-lasttime).dbl();
        currateVector.recordWithTimestamp(simTime(),currentRate);
        bitlength = 0;
        lasttime = simTime();
    }
    EV<<"packet length = "<<packet->getByteLength()<<endl;

    sendDown(packet);
    cancelEvent(timeout);
    scheduleAt(simTime() + RTO, timeout);


    if(sender_packetMap.find(nxtSendpacketid+1)==sender_packetMap.end()){
        EV<<"packet run out, stopping"<<endl;
        SenderState = STOPPING;
    }
    else if(snd_cwnd>=1){
        if(FAST_RECOVERY){
            if(!sacks_array_snd.empty())
            {
                sacks_array_snd.pop_front();
                nxtSendpacketid = sacks_array_snd.front();
                if(sacks_array_snd.empty())
                {
                    FAST_RECOVERY=false;
                    EV << "SACK array is empty"<<endl;
                    nxtSendpacketid = pre_snd;
                }
            }
        }else{
            nxtSendpacketid++;
        }
        if(snd_cwnd-(nxtSendpacketid-snd_una)>0){
            EV<<"snd_cwnd = "<<snd_cwnd<<", sended window - "<<packetid-snd_una<<endl;
            send_data();
        }
        else{
            EV<<"Pausing !!!!!"<<endl;
            SenderState = PAUSING;
        }
    }
    else if(snd_cwnd<1){
        if(FAST_RECOVERY){
            if(!sacks_array_snd.empty())
            {
                sacks_array_snd.pop_front();
                nxtSendpacketid = sacks_array_snd.front();
                if(sacks_array_snd.empty())
                {
                    FAST_RECOVERY=false;
                    EV << "SACK array is empty"<<endl;
                    nxtSendpacketid = pre_snd;
                }
            }
        }else{
            nxtSendpacketid++;
        }
        EV<<"snd_cwnd < 1, nxtSendtime = "<<simTime()+pacing_delay<<endl;
        cancelEvent(senddata);
        scheduleAt(simTime()+pacing_delay,senddata);
    }

}

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
            const auto& payload = makeShared<ByteCountChunk>(B(26));
            auto tag=payload->addTag<HiTag>();
            tag->setPacketId(curRcvNum);
            payload->enableImplicitChunkSerialization = true;
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
            payload->enableImplicitChunkSerialization = true;
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
            sendUp(pck);
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

            EV_DETAIL<<"packet is ordered, send ACK-"<<receiver_Map[srcAddr].rcv_nxt-1<<endl;
            std::ostringstream str;
            str <<"ACK-" <<receiver_Map[srcAddr].rcv_nxt-1;
            Packet *ack = new Packet(str.str().c_str());
            const auto& payload = makeShared<ByteCountChunk>(B(26));
            auto tag=payload->addTag<HiTag>();
            tag->setPacketId(receiver_Map[srcAddr].rcv_nxt-1);
            payload->enableImplicitChunkSerialization = true;
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
            while (it != receiver_Map[srcAddr].sacks_array.end()) {//删除小于rcv_nxt的序号
                if (it->getPacketid()<receiver_Map[srcAddr].rcv_nxt) {
                    EV_DETAIL << "\t SACK in sacks_array: " << " " << it->getPacketid() << " delete now\n";
                    it = receiver_Map[srcAddr].sacks_array.erase(it);
                }
                else {
                    EV_DETAIL << "\t SACK in sacks_array: " << " " << it->getPacketid() << endl;
                    it++;
                }
            }
            SackItem *sackItem = new SackItem;
            sackItem->setPacketid(curRcvNum);
            receiver_Map[srcAddr].sacks_array.push_front(*sackItem);//插入时后到在前，因为SACK需要先包含新的packetid

            it = receiver_Map[srcAddr].sacks_array.begin();
            for (; it != receiver_Map[srcAddr].sacks_array.end(); it++) {//删除重复
                auto it2 = it;
                it2++;
                while (it2 != receiver_Map[srcAddr].sacks_array.end()) {
                    if (it->getPacketid()==it2->getPacketid()) {
                        EV_DETAIL << "sack matched, delete contained : a=" << it->getPacketid() << ", b=" << it2->getPacketid() << endl;
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
            payload->enableImplicitChunkSerialization = true;
            sack->insertAtBack(payload);

            auto content = makeShared<SACK>();
            uint counter = 0;
            uint n = receiver_Map[srcAddr].sacks_array.size();
            if(n>4)
                n=4;//maxnode
            content->setSackItemArraySize(n);
            for (it = receiver_Map[srcAddr].sacks_array.begin(); it != receiver_Map[srcAddr].sacks_array.end() && counter < n; it++) {
                content->setSackItem(counter++, *it);//一般来说counter越小序号越大
            }

            sack->insertAtFront(content);

            auto addressReq = sack->addTagIfAbsent<L3AddressReq>();
            addressReq->setSrcAddress(desAddr);
            addressReq->setDestAddress(srcAddr);
            sack->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
            sack->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);
            sack->setTimestamp(ts);

            sendDown(sack);
            sendUp(pck);
        }
        else{
            delete pck;
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
    if(string(pck->getFullName()).find("SACK") != string::npos){
        auto SACK_msg = pck->peekAtFront<SACK>();
        uint n = SACK_msg->getSackItemArraySize();
        if(n>0){
            num_dupack++;
            for(uint i=0; i<n;i++){
                for(uint i2=ackid;i2<SACK_msg->getSackItem(i).getPacketid();i2++){
                    if(std::find(sacks_array_snd.begin(),sacks_array_snd.end(),i2)==sacks_array_snd.end())
                        sacks_array_snd.push_back(i2);
                }
            }
            for(uint i=0;i<n;i++){
                auto i2=sacks_array_snd.begin();
                while(i2!=sacks_array_snd.end()){
                    if(SACK_msg->getSackItem(i).getPacketid()==*i2)
                        i2 = sacks_array_snd.erase(i2);
                    else
                        i2++;
                }
            }
        }
        for(auto i=sacks_array_snd.begin();i!=sacks_array_snd.end();i++){
            EV<<"remain sack id = "<<*i<<endl;
        }
        if(num_dupack==3){
            retransmit_cnt = 0;
            num_dupack = 0;
            num_acked = 0;
            FAST_RECOVERY=true;
            pre_snd = nxtSendpacketid;EV<<"recovery begin, pre_snd = "<<pre_snd<<endl;
            nxtSendpacketid = sacks_array_snd.front();
            send_data();
        }
    }else if(ackid>=snd_una){
        snd_una = ackid+1;
        EV<<"snd_una update to "<<snd_una<<endl;
        num_dupack = 0;
        num_acked = 0;
        retransmit_cnt = 0;
        sender_packetMap.erase(ackid);EV<<"erase "<<ackid<<endl;
        auto i2=sacks_array_snd.begin();
        while(i2!=sacks_array_snd.end()){
            if(ackid>=*i2)
                i2 = sacks_array_snd.erase(i2);
            else
                i2++;
        }
        if(nxtSendpacketid<snd_una)
            nxtSendpacketid=snd_una;
        if(pre_snd<snd_una)
            pre_snd=snd_una;
    }


    simtime_t oldRTT_S = RTT_S;
    RTT_S = (1 - RTO_alpha) * oldRTT_S + RTO_alpha * currentRTT;
    RTT_D = (1 - RTO_beta) * RTT_D + RTO_beta * fabs(oldRTT_S - currentRTT);
    RTO = RTT_S + 4 * RTT_D;
    EV<<"RTO = "<<RTO<<endl;
    cancelEvent(timeout);

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

    snd_cwnd = math::clamp(temp_cwnd,min_cwnd,max_cwnd);
    EV<<"after changing cwnd is "<<snd_cwnd<<endl;
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
    EV<<"pacing_delay = "<<pacing_delay<<"s"<<endl;
    if(SenderState==PAUSING){
        SenderState=SENDING;
        cancelEvent(senddata);
        scheduleAt(simTime(),senddata);
    }
    delete pck;
}

void SWIFT::time_out()
{
    timeout_num++;
    retransmit_cnt++;
    EV<<"time out, retransmit_cnt = "<<retransmit_cnt<<endl;

    if(retransmit_cnt > RETX_RESET_THRESHOLD)
        snd_cwnd = min_cwnd;

    if(can_decrease)
    {
        snd_cwnd = (1 - max_mdf) * snd_cwnd;
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
        cancelEvent(timeout);
        scheduleAt(simTime(),senddata);  // 超时后，立即重发未确认的第一个报文
    }
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
