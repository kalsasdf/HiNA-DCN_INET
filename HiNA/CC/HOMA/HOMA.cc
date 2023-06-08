// Copyright (C)
/*
 * Developed by Lizihan
 * Begin at 2023/3/2
*/

#include "HOMA.h"
#include "inet/common/INETDefs.h"

namespace inet {

Define_Module(HOMA);

void HOMA::initialize(int stage)
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

        rttbytes = baseRTT.dbl()*linkspeed/8;

        setPrioCutOffs(); // set priority cutoffs for unscheduled packets

        receiver_flowMap.clear();
        sender_flowMap.clear();
    }else if (stage == INITSTAGE_TRANSPORT_LAYER) {
        registerService(Protocol::udp, gate("upperIn"), gate("upperOut"));
        registerProtocol(Protocol::udp, gate("lowerOut"), gate("lowerIn"));
    }
}

void HOMA::handleMessage(cMessage *msg)
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

void HOMA::handleSelfMessage(cMessage *pck)
{
    TimerMsg *timer = check_and_cast<TimerMsg *>(pck);
    // Process different self-messages (timer signals)
    EV_TRACE << "Self-message " << timer << " received\n";

    switch (timer->getKind()) {
        case SENDUNSCHEDULE:
            send_unschedule(timer->getFlowId());
            break;
        case SENDBUSY:
            send_busy(timer->getFlowId(), timer->getSeq());
            break;
        case SENDRESEND:
            send_resend(timer->getFlowId(), timer->getSeq());
            break;
        case TIMEOUT:
            time_out(timer->getFlowId());
            break;
    }
}


void HOMA::processUpperpck(Packet *pck)
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
            snd_info.crcMode = udpHeader->getCrcMode();
            snd_info.crc = udpHeader->getCrc();
            snd_info.sendRtt = 0;
            snd_info.unscheduledPrio = getMesgPrio(snd_info.unsSize);
            snd_info.sendunschedule = new TimerMsg("sendunschedule");
            snd_info.sendunschedule->setKind(SENDUNSCHEDULE);
            snd_info.sendunschedule->setFlowId(snd_info.flowid);
            if(snd_info.flowsize < rttbytes)
            {
                snd_info.unsSize = snd_info.flowsize;
                snd_info.sSize = 0;
            }
            else
            {
                snd_info.unsSize = rttbytes;
                snd_info.sSize = snd_info.flowsize - rttbytes;
            }

            sender_flowMap[flowid] = snd_info;

            scheduleAt(simTime(),snd_info.sendunschedule);
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

void HOMA::processLowerpck(Packet *pck)
{
    if (!strcmp(pck->getFullName(),"Grant"))
    {
        receive_grant(pck);
    }
    else if (!strcmp(pck->getFullName(),"Resend"))
    {
        receive_resend(pck);
    }
    else if (!strcmp(pck->getFullName(),"Busy"))
    {
        receive_busy(pck);
    }
    else if (string(pck->getFullName()).find("homaUData") != string::npos)
    {
        receive_unscheduledata(pck);
    }
    else if (string(pck->getFullName()).find("homaSData") != string::npos)
    {
        receive_scheduledata(pck);
    }
    else
    {
        sendUp(pck);
    }
}

void HOMA::send_unschedule(uint32_t flowid)
{
    sender_flowinfo snd_info = sender_flowMap.find(flowid)->second;
    std::ostringstream str;

    str << "homaUData" << "-" <<snd_info.flowid<< "-" <<snd_info.pckseq;
    Packet *packet = new Packet(str.str().c_str());
    int64_t this_pck_bytes = (snd_info.sendRtt+max_pck_size<=snd_info.unsSize)?max_pck_size:snd_info.unsSize-snd_info.sendRtt;
    snd_info.sendRtt += this_pck_bytes;

    const auto& payload = makeShared<ByteCountChunk>(B(this_pck_bytes));
    auto tag = payload->addTag<HiTag>();
    if(snd_info.sendRtt == snd_info.unsSize&&snd_info.sSize==0) tag->setIsLastPck(true);
    tag->setFlowId(snd_info.flowid);
    tag->setPacketId(snd_info.pckseq);
    if(lastflowid!=snd_info.flowid){
        last_creation_time = snd_info.cretime;
        lastflowid = snd_info.flowid;
    }
    tag->setCreationtime(last_creation_time);
    tag->setPriority(0);
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
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);
    packet->setKind(0);

    EV << "prepare to send unschedule packet, remaining unschedule data size = " << snd_info.unsSize-snd_info.sendRtt <<endl;
    sendDown(packet);
    sender_flowMap[flowid] = snd_info;
    if (snd_info.sendRtt < snd_info.unsSize)
    {
        send_unschedule(flowid);
        //发送自消息
//        scheduleAt(simTime(),snd_info.sendunschedule);
    }

}

void HOMA::receive_unscheduledata(Packet* pck)
{
   auto l3addr = pck->getTag<L3AddressInd>();
   srcAddr = l3addr->getDestAddress();
   L3Address destAddr = l3addr->getSrcAddress();
   int64_t length = pck->getByteLength();

   int flowid;
   bool last;
   int now_received_data_seq;
   uint flowsize;
   receiver_flowinfo rcv_info;

   //提取标签信息
   for (auto& region : pck->peekData()->getAllTags<HiTag>())
   {
       flowid = region.getTag()->getFlowId();
       last = region.getTag()->isLastPck();
       now_received_data_seq = region.getTag()->getPacketId();
       flowsize = region.getTag()->getFlowSize();
   }

   auto it = receiver_flowMap.find(flowid);
   if(it != receiver_flowMap.end())
   {
      rcv_info = it->second;
      rcv_info.unscheduleFlowSize += length;
   }
   else
   {
      rcv_info.destAddr = destAddr;
      rcv_info.unscheduleFlowSize = length;
      rcv_info.flowid = flowid;
      for(int i = 0; i < 8; i ++)
      {EV<<i<<" = "<<priorityUse[i]<<endl;
          if(!priorityUse[i])
          {
              rcv_info.SenderPriority = i;
              priorityUse[i] = true;
              break;
          }
      }
      rcv_info.sendresend = new TimerMsg("sendresend");
      rcv_info.sendresend->setKind(SENDRESEND);
      rcv_info.sendresend->setFlowId(flowid);
      rcv_info.timeout = new TimerMsg("timeout");
      rcv_info.timeout->setKind(TIMEOUT);
      rcv_info.timeout->setFlowId(flowid);
   }
   rcv_info.now_received_data_seq = now_received_data_seq;
   rcv_info.now_send_grt_seq = -1;
   rcv_info.scheduleFlowSize = 0;
   rcv_info.if_get[rcv_info.now_received_data_seq] = true;
   if(last) {
       rcv_info.ReceiverState = GRANT_STOP;
       priorityUse[rcv_info.SenderPriority] = false;
       cancelEvent(rcv_info.sendresend);
       delete rcv_info.sendresend;
       cancelEvent(rcv_info.timeout);
       delete rcv_info.timeout;
       receiver_flowMap.erase(flowid);
   }
   else {
       rcv_info.ReceiverState = GRANT_SENDING;
       receiver_flowMap[flowid] = rcv_info;
   }

   if(rcv_info.ReceiverState == GRANT_SENDING)
   {
       EV << "The flow " << flowid << " unschedule data received, send GRANT" << endl;

       send_grant(flowid);
   }

   //检测是否发生丢包（在超时时间内未收到即认为丢包，此时需要发送RESEND指令
   sendUp(pck);
}

void HOMA::send_grant(uint32_t flowid)
{
    receiver_flowinfo rcv_info = receiver_flowMap[flowid];

    if(simTime()<stopTime){
        Packet *grant = new Packet("Grant");

        rcv_info.now_send_grt_seq ++;
        const auto& content = makeShared<ByteCountChunk>(B(26));
        auto tag = content->addTag<HiTag>();
        tag->setPriority(0);
        tag->setFlowId(rcv_info.flowid);
        tag->setPacketId(rcv_info.now_send_grt_seq);
        tag->setSenderPriority(rcv_info.SenderPriority);
        content->enableImplicitChunkSerialization=true;
        grant->insertAtBack(content);

        grant->addTagIfAbsent<L3AddressReq>()->setDestAddress(rcv_info.destAddr);
        grant->addTagIfAbsent<L3AddressReq>()->setSrcAddress(srcAddr);
        grant->setTimestamp(simTime());
        grant->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
        grant->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

        sendDown(grant);
        EV<<"send grant "<<rcv_info.now_send_grt_seq<<", timestamp = "<<simTime()<<"s, "<< "for flow " << flowid << endl;

        //检测是否发生丢包（在超时时间内未收到即认为丢包，此时需要发送RESEND指令
        cancelEvent(rcv_info.timeout);
        scheduleAt(simTime() + timeout, rcv_info.timeout);

        receiver_flowMap[flowid] = rcv_info;
    }else{
        cancelEvent(rcv_info.timeout);
        receiver_flowMap[flowid] = rcv_info;
    }

}

void HOMA::receive_grant(Packet *pck)
{
    auto l3addr = pck->getTag<L3AddressInd>();
    EV<<"receive_grant(), the source "<<l3addr->getSrcAddress()<<endl;

    long packetid;
    int priority;
    uint32_t flowid;
    for (auto& region : pck->peekData()->getAllTags<HiTag>())
    {
        packetid = region.getTag()->getPacketId();
        priority = region.getTag()->getSenderPriority();
        flowid = region.getTag()->getFlowId();
        EV << "the flow " << flowid << ", grant_id = "<< packetid <<endl;
    }
    sender_flowinfo snd_info = sender_flowMap.find(flowid)->second;

    if(simTime()<stopTime&&snd_info.sSize>0)
    {
        snd_info.flowid = flowid;
        int64_t this_pck_bytes = 0;
        bool last = false;
        std::ostringstream str;
        str << "homaSData" << "-" << flowid << "-" << snd_info.pckseq << endl;
        Packet *packet = new Packet(str.str().c_str());
        this_pck_bytes = max_pck_size;
        if(snd_info.sSize <= this_pck_bytes)
        {
            this_pck_bytes = snd_info.sSize;
            snd_info.sSize = 0;
            last = true;
        }
        else
        {
            snd_info.sSize -= this_pck_bytes;
        }
        const auto& payload = makeShared<ByteCountChunk>(B(this_pck_bytes));
        auto tag = payload->addTag<HiTag>();
        tag->setFlowId(snd_info.flowid);
        tag->setPacketId(snd_info.pckseq);
        tag->setCreationtime(snd_info.cretime);
        tag->setPriority(priority);
        tag->setIsLastPck(last);
        packet->insertAtBack(payload);
        snd_info.pckseq ++;

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
        packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);
        packet->setKind(0);

        EV << "prepare to send packet, remaining data size = " << snd_info.sSize <<endl;
        sender_flowMap[flowid] = snd_info;
        sendDown(packet);
    }
    delete(pck);
}

void HOMA::receive_scheduledata(Packet* pck)
{
    auto l3addr = pck->getTag<L3AddressInd>();
    L3Address srcAddr = l3addr->getDestAddress();
    L3Address destAddr = l3addr->getSrcAddress();
    int64_t length = pck->getByteLength();

    bool last;
    int flowid;
    int priority;
    int now_received_data_seq;
    //提取标签信息
    for (auto& region : pck->peekData()->getAllTags<HiTag>())
    {
       last = region.getTag()->isLastPck();
       flowid = region.getTag()->getFlowId();
       now_received_data_seq = region.getTag()->getPacketId();
       priority = region.getTag()->getPriority();
    }
    receiver_flowinfo rcv_info = receiver_flowMap.find(flowid) -> second;
    //解除该优先级的限制
    rcv_info.now_received_data_seq = now_received_data_seq;

    rcv_info.scheduleFlowSize += length;
    rcv_info.if_get[rcv_info.now_received_data_seq] = true;
    if(last){
        rcv_info.ReceiverState = GRANT_STOP;
        priorityUse[priority] = false;
        cancelEvent(rcv_info.sendresend);
        delete rcv_info.sendresend;
        cancelEvent(rcv_info.timeout);
        delete rcv_info.timeout;
        receiver_flowMap.erase(flowid);
    }
    else receiver_flowMap[flowid] = rcv_info;

    if(rcv_info.ReceiverState == GRANT_SENDING)
    {
        EV << "The flow " << flowid << " unschedule data has been all received, send GRANT" << endl;

        send_grant(flowid);

    }

    sendUp(pck);
}

void HOMA::send_resend(uint32_t flowid, long seq)
{
    receiver_flowinfo rcv_info = receiver_flowMap[flowid];
    Packet *resend = new Packet("Resend");

    const auto& content = makeShared<ByteCountChunk>(B(1));
    auto tag = content -> addTag<HiTag>();
    tag->setPriority(0);
    tag->setFlowId(rcv_info.flowid);
    //此时packetid为期待重传的包
    tag->setPacketId(seq);
    resend->insertAtFront(content);

    resend->addTagIfAbsent<L3AddressReq>()->setDestAddress(rcv_info.destAddr);
    resend->addTagIfAbsent<L3AddressReq>()->setSrcAddress(srcAddr);
    resend->setTimestamp(simTime());
    resend->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    resend->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

    sendDown(resend);
    EV << "send resend, flow_id = " << rcv_info.flowid << ", the miss pckid = " << seq << ", timestamp = " << simTime() << endl;
}

void HOMA::receive_resend(Packet* pck)
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
    //若此时正在传输数据，发送BUSY包
//    if(snd_info.SenderState == US_SENT)
//    {
//        send_busy(flowid, seq);
//    }
//    else
//    {
        std::ostringstream str;
        str << "homaScheduleData" << "-" << flowid << "-" <<seq;
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
        delete pck;
//    }
}

void HOMA::send_busy(uint32_t flowid, long seq)
{
    sender_flowinfo snd_info = sender_flowMap[flowid];
    Packet *busy = new Packet("Busy");

    const auto& content = makeShared<ByteCountChunk>(B(1));
    auto tag = content -> addTag<HiTag>();
    tag->setPriority(7);
    tag->setFlowId(snd_info.flowid);
    //此时packetid为期待重传的包
    tag->setPacketId(seq);
    busy->insertAtFront(content);

    busy->addTagIfAbsent<L3AddressReq>()->setDestAddress(snd_info.destAddr);
    busy->addTagIfAbsent<L3AddressReq>()->setSrcAddress(srcAddr);
    busy->setTimestamp(simTime());
    busy->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    busy->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);

    sendDown(busy);
}

void HOMA::receive_busy(Packet* pck)
{
    auto l3addr = pck->addTagIfAbsent<L3AddressInd>();
    long seq;
    uint32_t flowid;
    for (auto& region : pck->peekData()->getAllTags<HiTag>())
    {
       seq = region.getTag()->getPacketId();
       flowid = region.getTag()->getFlowId();
    }
    receiver_flowinfo rcv_info = receiver_flowMap[flowid];
    rcv_info.sendresend->setSeq(seq);
    scheduleAt(simTime() + timeout, rcv_info.sendresend);
}



void HOMA::time_out(uint32_t flowid)
{
    receiver_flowinfo rcv_info = receiver_flowMap.find(flowid) -> second;
    if(rcv_info.ReceiverState!=GRANT_STOP){
        for(int i = 0; i <= 10000; i ++)
        {
            if(rcv_info.if_get[i] == false)
            {
                send_resend(flowid, i);
                break;
            }
        }
    }
}

void HOMA::sendDown(Packet *pck)
{
    EV << "sendDown()" <<endl;
    send(pck,outGate);
}

void HOMA::sendUp(Packet *pck)
{
    EV<<"HOMA, oh sendup!"<<endl;
    send(pck,upGate);
}

void HOMA::setPrioCutOffs()
{
    prioCutOffs.push_back(0);
    prioCutOffs.push_back(50);
    prioCutOffs.push_back(70);
    prioCutOffs.push_back(100);
    prioCutOffs.push_back(200);
    prioCutOffs.push_back(10000);
}

uint16_t HOMA::getMesgPrio(uint32_t msgSize)
{
    size_t mid, high, low;
    low = 0;
    high = prioCutOffs.size() - 1;
    while(low < high)
    {
        mid = (high + low) / 2;
        if (msgSize <= prioCutOffs.at(mid))
        {
            high = mid;
        }
        else
        {
            low = mid + 1;
        }
    }
    return high;
}

void HOMA::refreshDisplay() const
{

}

void HOMA::finish()
{

}
}//namespace inet;


