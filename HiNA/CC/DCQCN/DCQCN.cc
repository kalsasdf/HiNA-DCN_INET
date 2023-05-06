// Copyright (C)
/*
 * Developed by Angrydudu
 * Begin at 2020/03/02
*/
#include "DCQCN.h"

namespace inet {

Define_Module(DCQCN);

void DCQCN::initialize(int stage)
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
        gamma = par("gamma");
        linkspeed = par("linkspeed");
        initialrate = par("initialrate");
        min_cnp_interval = par("min_cnp_interval");
        AlphaTimer_th = par("AlphaTimer_th");
        RateTimer_th = par("RateTimer_th");
        ByteCounter_th = par("ByteCounter_th");
        frSteps_th = par("frSteps_th");
        Rai = par("Rai");
        Rhai = par("Rhai");
        max_pck_size=par("max_pck_size");
        senddata = new TimerMsg("senddata");
        senddata->setKind(SENDDATA);
    }else if (stage == INITSTAGE_TRANSPORT_LAYER) {
        registerService(Protocol::udp, gate("upperIn"), gate("upperOut"));
        registerProtocol(Protocol::udp, gate("lowerOut"), gate("lowerIn"));
    }
}

void DCQCN::handleMessage(cMessage *msg)
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

void DCQCN::handleSelfMessage(cMessage *pck)
{
    // Process different self-messages (timer signals)
    TimerMsg *timer = check_and_cast<TimerMsg *>(pck);
    EV_TRACE << "Self-message " << timer << " received, type = "<<timer->getKind()<<endl;
    switch (timer->getKind()) {
        case SENDDATA:
        {
            if(sender_flowMap.empty()||simTime()>=stopTime){
                SenderState=STOPPING;
                break;
            }else{
                send_data();
                break;
            }
        }
        case RATETIMER:
        {
            sender_flowinfo sndinfo = sender_flowMap.find(timer->getFlowId())->second;
            sndinfo.TimeFrSteps++;
            increaseTxRate(timer->getFlowId());
            cancelEvent(sndinfo.rateTimer);
            sndinfo.rateTimer->setKind(RATETIMER);
            scheduleAt(simTime()+RateTimer_th,sndinfo.rateTimer);
            break;
        }
        case ALPHATIMER:
        {
            updateAlpha(timer->getFlowId());
            break;
        }
    }
}

// Record the packet from app to transmit it to the dest
void DCQCN::processUpperpck(Packet *pck)
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
        if(sender_flowMap.find(flowid)!=sender_flowMap.end()){
            sender_flowMap[flowid].remainLength+=pck->getByteLength();
        }else{
            sender_flowinfo snd_info;
            snd_info.flowid = flowid;
            EV << "store new flow, id = "<<snd_info.flowid<<
                    ", creationtime = "<<snd_info.cretime<<endl;
            auto addressReq = pck->addTagIfAbsent<L3AddressReq>();
            srcAddr = addressReq->getSrcAddress();
            L3Address destAddr = addressReq->getDestAddress();

            auto udpHeader = pck->removeAtFront<UdpHeader>();

            snd_info.remainLength = pck->getByteLength();
            snd_info.destAddr = destAddr;
            snd_info.srcPort = udpHeader->getSrcPort();
            snd_info.destPort = udpHeader->getDestPort();
            snd_info.priority = priority;
            snd_info.pckseq = 0;
            snd_info.crcMode = udpHeader->getCrcMode();
            snd_info.crc = udpHeader->getCrc();
            snd_info.currentRate = linkspeed*initialrate;
            snd_info.targetRate = snd_info.currentRate;
            snd_info.rateTimer =  new TimerMsg("rateTimer");
            snd_info.alphaTimer = new TimerMsg("alphaTimer");
            if(sender_flowMap.empty()){
                sender_flowMap[snd_info.flowid]=snd_info;
                iter=sender_flowMap.begin();//iter needs to be assigned after snd_info is inserted
            }else{
                sender_flowMap[snd_info.flowid]=snd_info;
            }
        }
        delete pck;
        if(SenderState==STOPPING){
            SenderState=SENDING;
            scheduleAt(simTime(),senddata);
        }
    }
    else
    {
        EV<<"Unknown packet, sendDown()."<<endl;
        sendDown(pck);
    }
}

void DCQCN::send_data()
{
    sender_flowinfo snd_info = iter->second;
    std::ostringstream str;
    str << packetName << "-" <<snd_info.flowid<<"-" <<snd_info.pckseq;
    Packet *packet = new Packet(str.str().c_str());
    int pcklength;
    if (snd_info.remainLength > max_pck_size)
    {
        pcklength=max_pck_size;
        snd_info.remainLength = snd_info.remainLength - max_pck_size;
    }
    else
    {
        pcklength=snd_info.remainLength;
        snd_info.remainLength = 0;
    }
    const auto& payload = makeShared<ByteCountChunk>(B(pcklength));
    auto tag = payload->addTag<HiTag>();
    tag->setFlowId(snd_info.flowid);
    tag->setPriority(snd_info.priority);
    tag->setCreationtime(snd_info.cretime);
    tag->setPacketId(snd_info.pckseq);
    snd_info.pckseq += 1;

    packet->insertAtBack(payload);

    auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
    addressReq->setSrcAddress(srcAddr);
    addressReq->setDestAddress(snd_info.destAddr);

    // insert udpHeader, set source and destination port
    const Protocol *l3Protocol = &Protocol::ipv4;
    auto udpHeader = makeShared<UdpHeader>();
    udpHeader->setSourcePort(snd_info.srcPort);
    udpHeader->setDestinationPort(snd_info.destPort);
    udpHeader->setCrc(snd_info.crc);
    udpHeader->setCrcMode(snd_info.crcMode);
    udpHeader->setTotalLengthField(udpHeader->getChunkLength() + packet->getTotalLength());
    insertTransportProtocolHeader(packet, Protocol::udp, udpHeader);
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);
    packet->addTagIfAbsent<EcnReq>()->setExplicitCongestionNotification(1);
    packet->setKind(0);

    EV<<"packet length = "<<packet->getByteLength()<<", current rate = "<<snd_info.currentRate<<endl;
    EV << "prepare to send packet, remaining data size = " << snd_info.remainLength <<endl;

    if (snd_info.remainLength == 0)
    {
        cancelEvent(snd_info.rateTimer);
        delete snd_info.rateTimer;
        cancelEvent(snd_info.alphaTimer);
        delete snd_info.alphaTimer;
        iter++;
        sender_flowMap.erase(snd_info.flowid);
    }else{
        if (snd_info.SenderAcceleratingState != Normal)
        {
            snd_info.ByteCounter += packet->getByteLength();
            if (snd_info.ByteCounter >= ByteCounter_th)
            {
                snd_info.ByteFrSteps++;
                snd_info.ByteCounter = 0;
                EV<<"byte counter expired, byte fr steps = "<<snd_info.ByteFrSteps<<endl;
                increaseTxRate(snd_info.flowid);
            }
        }
        sender_flowMap[snd_info.flowid] =snd_info;
    }
    simtime_t d = simtime_t((packet->getByteLength()+58)*8/snd_info.currentRate);EV<<"send interval = "<<d<<endl;
    simtime_t nxtSendTime = simtime_t((packet->getByteLength()+58)*8/snd_info.currentRate) + simTime();
    //58=20(IP)+14(EthernetMac)+8(EthernetPhy)+4(EthernetFcs)+12(interframe gap,IFG)
    scheduleAt(nxtSendTime,senddata);
    sendDown(packet);
}

void DCQCN::processLowerpck(Packet *pck)
{
    if (!strcmp(pck->getFullName(),"CNP"))
    {
        receive_cnp(pck);
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

void DCQCN::receive_data(Packet *pck)
{
    auto l3AddressInd = pck->getTag<L3AddressInd>();
    auto rcv_srcAddr = l3AddressInd->getSrcAddress();
    auto rcv_destAddr = l3AddressInd->getDestAddress();
    auto ecn = pck->addTagIfAbsent<EcnInd>()->getExplicitCongestionNotification();
    uint flowid;
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        flowid = region.getTag()->getFlowId();
    }
    EV<<"receive packet, ecn = "<<ecn<<endl;
    if (ecn == 3&&simTime()-lastCnpTime[flowid]>min_cnp_interval) // ecn==1, enabled; ecn==3, marked.
    {
        Packet *cnp = new Packet("CNP");
        const auto& payload = makeShared<ByteCountChunk>(B(1));
        auto tag = payload->addTag<HiTag>();
        tag->setFlowId(flowid);
        cnp->insertAtBack(payload);
        cnp->addTagIfAbsent<L3AddressReq>()->setDestAddress(rcv_srcAddr);
        cnp->addTagIfAbsent<L3AddressReq>()->setSrcAddress(rcv_destAddr);
        cnp->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
        cnp->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);
        sendDown(cnp);
        EV_INFO << "Sending CNP packet to "<<rcv_srcAddr<<", cnp interval = "<<simTime()-lastCnpTime[flowid]<<endl;
        lastCnpTime[flowid] = simTime();
    }
    sendUp(pck);
}

void DCQCN::receive_cnp(Packet *pck)
{
    uint32_t flowid;
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        flowid = region.getTag()->getFlowId();
    }
    if(sender_flowMap.find(flowid)!=sender_flowMap.end()){
        sender_flowinfo sndinfo = sender_flowMap.find(flowid)->second;
        // cut rate
        sndinfo.targetRate = sndinfo.currentRate;
        sndinfo.currentRate = sndinfo.currentRate * (1 - sndinfo.alpha/2);
        sndinfo.alpha = (1 - gamma) * sndinfo.alpha + gamma;
        EV<<"after cutting, the current rate = "<<sndinfo.currentRate<<
                ", target rate = "<<sndinfo.targetRate<<endl;

        // reset timers and counter
        sndinfo.ByteCounter = 0;
        sndinfo.ByteFrSteps = 0;
        sndinfo.TimeFrSteps = 0;
        sndinfo.iRhai = 0;
        cancelEvent(sndinfo.alphaTimer);
        cancelEvent(sndinfo.rateTimer);

        sndinfo.alphaTimer->setKind(ALPHATIMER);

        sndinfo.rateTimer->setKind(RATETIMER);
        sndinfo.rateTimer->setFlowId(flowid);
        // update alpha
        scheduleAt(simTime()+AlphaTimer_th,sndinfo.alphaTimer);

        // schedule to rate increase event
        scheduleAt(simTime()+RateTimer_th,sndinfo.rateTimer);
    }
    delete pck;
}

void DCQCN::increaseTxRate(uint32_t flowid)
{
    sender_flowinfo sndinfo = sender_flowMap.find(flowid)->second;

    if (max(sndinfo.ByteFrSteps,sndinfo.TimeFrSteps) < frSteps_th)
    {
        sndinfo.SenderAcceleratingState = Fast_Recovery;
    }
    else if (min(sndinfo.ByteFrSteps,sndinfo.TimeFrSteps) > frSteps_th)
    {
        sndinfo.SenderAcceleratingState = Hyper_Increase;
    }
    else
    {
        sndinfo.SenderAcceleratingState = Additive_Increase;
    }
    EV<<"entering incease rate, sender state = "<<sndinfo.SenderAcceleratingState<<endl;

    if (sndinfo.SenderAcceleratingState == Fast_Recovery)
    {// Fast Recovery
        sndinfo.currentRate = (sndinfo.currentRate + sndinfo.targetRate) / 2;
    }
    else if (sndinfo.SenderAcceleratingState == Additive_Increase)
    {// Additive Increase
        sndinfo.targetRate += Rai;
        sndinfo.targetRate = (sndinfo.targetRate > sndinfo.maxTxRate) ? sndinfo.maxTxRate : sndinfo.targetRate;

        sndinfo.currentRate = (sndinfo.currentRate + sndinfo.targetRate) / 2;
    }
    else if (sndinfo.SenderAcceleratingState == Hyper_Increase)
    {// Hyper Increase
        sndinfo.iRhai++;
        sndinfo.targetRate += sndinfo.iRhai*Rhai;
        sndinfo.targetRate = (sndinfo.targetRate > sndinfo.maxTxRate) ? sndinfo.maxTxRate : sndinfo.targetRate;
        sndinfo.currentRate = (sndinfo.currentRate + sndinfo.targetRate) / 2;
    }
    EV<<"after increasing, the current rate = "<<sndinfo.currentRate<<
            ", target rate = "<<sndinfo.targetRate<<endl;
    sender_flowMap[flowid] = sndinfo;
}

void DCQCN::updateAlpha(uint32_t flowid)
{
    sender_flowinfo sndinfo = sender_flowMap.find(flowid)->second;
    sndinfo.alpha = (1 - gamma) * sndinfo.alpha;
    sender_flowMap[flowid] = sndinfo;
    scheduleAt(simTime() + AlphaTimer_th, sndinfo.alphaTimer);
}

void DCQCN::sendDown(Packet *pck)
{
    EV << "sendDown " <<pck->getFullName()<<endl;
    send(pck,lowerOutGate);
}

void DCQCN::sendUp(Packet *pck)
{
    EV<<"sendup"<<endl;
    send(pck,upperOutGate);
}

void DCQCN::refreshDisplay() const
{

}

void DCQCN::finish()
{

}

} // namespace inet
