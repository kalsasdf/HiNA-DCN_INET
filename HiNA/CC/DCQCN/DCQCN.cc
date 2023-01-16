// Copyright (C)
/*
 * Developed by Angrydudu
 * Begin at 2020/03/02
*/
#include "DCQCN.h"

namespace inet {

Define_Module(DCQCN);

void DCQCN::initialize()
{
    //statistics
    lowerOutGate = gate("lowerOut");
    lowerInGate = gate("lowerIn");
    upperOutGate = gate("upperOut");
    upperInGate = gate("upperIn");
    // configuration
    gamma = par("gamma");
    linkspeed = par("linkspeed");
    min_cnp_interval = par("min_cnp_interval");
    AlphaTimer_th = par("AlphaTimer_th");
    RateTimer_th = par("RateTimer_th");
    ByteCounter_th = par("ByteCounter_th");
    frSteps_th = par("frSteps_th");
    Rai = par("Rai");
    Rhai = par("Rhai");
    currentRate = linkspeed;
    targetRate = linkspeed;
    rateTimer =  new TimerMsg("rateTimer");
    alphaTimer = new TimerMsg("alphaTimer");

    registerService(Protocol::udp, gate("upperIn"), gate("upperOut"));
    registerProtocol(Protocol::udp, gate("lowerOut"), gate("lowerIn"));
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
            if(sender_flowMap.empty()){
                SenderState=STOPPING;
                break;
            }else{
                send_data();
                break;
            }
        }
        case RATETIMER:
        {
            TimeFrSteps++;
            increaseTxRate(timer->getDestAddr());
            cancelEvent(rateTimer);
            rateTimer->setKind(RATETIMER);
            rateTimer->setDestAddr(timer->getDestAddr());
            scheduleAt(simTime()+RateTimer_th,rateTimer);
            break;
        }
        case ALPHATIMER:
        {
            updateAlpha();
            break;
        }
    }
}

void DCQCN::refreshDisplay() const
{

}

// Record the packet from app to transmit it to the dest
void DCQCN::processUpperpck(Packet *pck)
{
    if (string(pck->getFullName()).find("Data") != string::npos){
        sender_flowinfo snd_info;
        for (auto& region : pck->peekData()->getAllTags<HiTag>()){
            snd_info.flowid = region.getTag()->getFlowId();
            snd_info.cretime = region.getTag()->getCreationtime();
            snd_info.priority = region.getTag()->getPriority();
            snd_info.flowsize = region.getTag()->getFlowSize();
            EV << "store new flow, id = "<<snd_info.flowid<<
                    ", creationtime = "<<snd_info.cretime<<endl;
        }
        auto addressReq = pck->addTagIfAbsent<L3AddressReq>();
        srcAddr = addressReq->getSrcAddress();
        L3Address destAddr = addressReq->getDestAddress();

        if(SenderState==STOPPING){
            TimerMsg *senddata = new TimerMsg("senddata");
            senddata->setKind(SENDDATA);
            senddata->setDestAddr(destAddr);
            senddata->setFlowId(snd_info.flowid);
            SenderState=SENDING;
            scheduleAt(simTime(),senddata);
        }
        auto udpHeader = pck->removeAtFront<UdpHeader>();

        snd_info.destAddr = destAddr;
        snd_info.srcPort = udpHeader->getSrcPort();
        snd_info.destPort = udpHeader->getDestPort();
        snd_info.crcMode = udpHeader->getCrcMode();
        snd_info.crc = udpHeader->getCrc();
        sender_flowMap[snd_info.flowid] =snd_info;
        // send packet timer
        delete pck;
    }
    else
    {
        EV<<"Unknown packet, sendDown()."<<endl;
        sendDown(pck);
    }
}

void DCQCN::send_data()
{
    sender_flowinfo snd_info = sender_flowMap.begin()->second;
    std::ostringstream str;
    str << packetName << "-" <<snd_info.flowid;
    Packet *packet = new Packet(str.str().c_str());
    const auto& payload = makeShared<ByteCountChunk>(B(snd_info.flowsize));
    auto tag = payload->addTag<HiTag>();
    tag->setFlowId(snd_info.flowid);
    tag->setPriority(snd_info.priority);
    tag->setCreationtime(snd_info.cretime);

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

    EV<<"packet length = "<<packet->getByteLength()<<", current rate = "<<currentRate<<endl;
    nxtSendTime = (long)((packet->getByteLength()+70)*8/currentRate) + simTime();
    TimerMsg *senddata = new TimerMsg("senddata");
    senddata->setKind(SENDDATA);
    senddata->setDestAddr(snd_info.destAddr);
    senddata->setFlowId(snd_info.flowid);
    if (SenderAcceleratingState != Normal)
    {
        ByteCounter += packet->getByteLength();
        if (ByteCounter >= ByteCounter_th)
        {
            ByteFrSteps++;
            ByteCounter = 0;
            EV<<"byte counter expired, byte fr steps = "<<ByteFrSteps<<endl;
            increaseTxRate(snd_info.destAddr);
        }
    }
    scheduleAt(nxtSendTime,senddata);
    sendDown(packet);
    sender_flowMap.erase(snd_info.flowid);
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
    {EV<<"send up"<<endl;
        sendUp(pck);
    }
}

void DCQCN::receive_data(Packet *pck)
{
    auto l3AddressInd = pck->getTag<L3AddressInd>();
    auto srcAddr = l3AddressInd->getSrcAddress();
    auto destAddr = l3AddressInd->getDestAddress();
    auto ecn = pck->addTagIfAbsent<EcnInd>()->getExplicitCongestionNotification();

    if (ecn == 3&&simTime()-lastCnpTime>min_cnp_interval) // ecn==1, enabled; ecn==3, marked.
    {
        send_cnp(srcAddr);
    }
    EV<<"receive packet, "<<", ecn = "<<ecn<<endl;
    sendUp(pck);
}

void DCQCN::send_cnp(L3Address destaddr)
{
    Packet *cnp = new Packet("CNP");

    cnp->addTagIfAbsent<L3AddressReq>()->setDestAddress(destaddr);
    cnp->addTagIfAbsent<L3AddressReq>()->setSrcAddress(srcAddr);
    cnp->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    cnp->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);
    EV_INFO << "Sending CNP packet to "<<destaddr <<", cnp interval = "<<simTime()-lastCnpTime<<endl;
    lastCnpTime = simTime();

    sendDown(cnp);
}

void DCQCN::receive_cnp(Packet *pck)
{
    L3Address destAddr = pck->addTagIfAbsent<L3AddressInd>()->getSrcAddress();

    // cut rate
    targetRate = currentRate;
    currentRate = currentRate * (1 - alpha/2);
    alpha = (1 - gamma) * alpha + gamma;
    EV<<"after cutting, the current rate = "<<currentRate<<
            ", target rate = "<<targetRate<<endl;

    // reset timers and counter
    ByteCounter = 0;
    ByteFrSteps = 0;
    TimeFrSteps = 0;
    iRhai = 0;
    cancelEvent(alphaTimer);
    cancelEvent(rateTimer);

    alphaTimer->setKind(ALPHATIMER);

    rateTimer->setKind(RATETIMER);
    rateTimer->setDestAddr(destAddr);
    // update alpha
    scheduleAt(simTime()+AlphaTimer_th,alphaTimer);

    // schedule to rate increase event
    scheduleAt(simTime()+RateTimer_th,rateTimer);
}

void DCQCN::increaseTxRate(L3Address destaddr)
{
    double oldrate = currentRate;

    if (max(ByteFrSteps,TimeFrSteps) < frSteps_th)
    {
        SenderAcceleratingState = Fast_Recovery;
    }
    else if (min(ByteFrSteps,TimeFrSteps) > frSteps_th)
    {
        SenderAcceleratingState = Hyper_Increase;
    }
    else
    {
        SenderAcceleratingState = Additive_Increase;
    }
    EV<<"entering incease rate, sender state = "<<SenderAcceleratingState<<endl;

    if (SenderAcceleratingState == Fast_Recovery)
    {// Fast Recovery
        currentRate = (currentRate + targetRate) / 2;
    }
    else if (SenderAcceleratingState == Additive_Increase)
    {// Additive Increase
        targetRate += Rai;
        targetRate = (targetRate > maxTxRate) ? maxTxRate : targetRate;

        currentRate = (currentRate + targetRate) / 2;
    }
    else if (SenderAcceleratingState == Hyper_Increase)
    {// Hyper Increase
        iRhai++;
        targetRate += iRhai*Rhai;
        targetRate = (targetRate > maxTxRate) ? maxTxRate : targetRate;
        currentRate = (currentRate + targetRate) / 2;
    }
    EV<<"after increasing, the current rate = "<<currentRate<<
            ", target rate = "<<targetRate<<endl;
}

void DCQCN::updateAlpha()
{
    alpha = (1 - gamma) * alpha;
    scheduleAt(simTime() + AlphaTimer_th, alphaTimer);
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

void DCQCN::finish()
{

}

} // namespace inet
