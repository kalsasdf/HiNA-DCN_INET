/*
 * TIMELY.cc
 *
 *  Created on: 20230205
 *      Author: luca
 */
#include "TIMELY.h"

namespace inet {

Define_Module(TIMELY);

void TIMELY::initialize(int stage)
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
        minRTT = par("minRTT");
        Tlow = par("Tlow");
        Thigh = par("Thigh");
        linkspeed = par("linkspeed");
        Rai = par("Rai");
        max_pck_size = par("max_pck_size");
        alpha = par("alpha");
        beta = par("beta");
        TIMELYseg=par("TIMELYseg");
        currentRate = linkspeed;

        senddata = new TimerMsg("senddata");
        senddata->setKind(SENDDATA);
    }else if (stage == INITSTAGE_TRANSPORT_LAYER) {
        registerService(Protocol::udp, gate("upperIn"), gate("upperOut"));
        registerProtocol(Protocol::udp, gate("lowerOut"), gate("lowerIn"));
    }
}

void TIMELY::handleMessage(cMessage *msg)
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

void TIMELY::handleSelfMessage(cMessage *pck)
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
    }
}

void TIMELY::processUpperpck(Packet *pck)
{
    if (string(pck->getFullName()).find("Data") != string::npos&&activate==true){
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
            srcAddr = addressReq->getSrcAddress();
            L3Address destAddr = addressReq->getDestAddress();
            auto udpHeader = pck->removeAtFront<UdpHeader>();

            snd_info.flowid = flowid;
            snd_info.remainLength = messageLength;
            snd_info.cretime = cretime;
            snd_info.destAddr = destAddr;
            snd_info.srcPort = udpHeader->getSrcPort();
            snd_info.destPort = udpHeader->getDestPort();
            snd_info.priority = priority;
            snd_info.pckseq = 0;
            snd_info.crcMode = udpHeader->getCrcMode();
            snd_info.crc = udpHeader->getCrc();
            if(sender_flowMap.empty()){
                sender_flowMap[snd_info.flowid]=snd_info;
                iter=sender_flowMap.begin();//iter needs to be assigned after snd_info is inserted
            }else{
                sender_flowMap[snd_info.flowid]=snd_info;
            }
            EV << "store new flow, id = "<<snd_info.flowid<<
                    ", creationtime = "<<snd_info.cretime<<endl;
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

void TIMELY::send_data()
{
    sender_flowinfo snd_info = iter->second;
    std::ostringstream str;
    str << packetName << "-" <<snd_info.flowid<< "-" <<snd_info.pckseq;
    Packet *packet = new Packet(str.str().c_str());
    int pcklength;
    if (accumlength+max_pck_size>TIMELYseg)
    {
        if(snd_info.remainLength <= TIMELYseg-accumlength){
            isLastPck = false;
            pcklength = snd_info.remainLength;
            snd_info.remainLength = 0;
        }else{
            isLastPck = true;
            pcklength = TIMELYseg-accumlength;
            snd_info.remainLength = snd_info.remainLength - pcklength;
        }
    }
    else
    {
        isLastPck = false;
        if(snd_info.remainLength <= max_pck_size){
            pcklength = snd_info.remainLength;
            snd_info.remainLength = 0;
        }else{
            pcklength = max_pck_size;
            snd_info.remainLength = snd_info.remainLength - max_pck_size;
        }
    }
    const auto& payload = makeShared<ByteCountChunk>(B(pcklength));
    auto tag = payload->addTag<HiTag>();
    tag->setFlowId(snd_info.flowid);
    tag->setPriority(snd_info.priority);
    if(lastflowid!=snd_info.flowid){
        last_creation_time = snd_info.cretime;
        lastflowid = snd_info.flowid;
    }
    tag->setCreationtime(last_creation_time);
    tag->setPacketId(snd_info.pckseq);
    tag->setIsLastPck(isLastPck);
    snd_info.pckseq += 1;
    segcount += 1;
    accumlength += pcklength;

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
    packet->setKind(0);

    EV<<"packet length = "<<packet->getByteLength()<<", current rate = "<<currentRate<<endl;
    EV << "prepare to send packet, remaining data size = " << snd_info.remainLength <<endl;

    if (snd_info.remainLength == 0){
        iter++;
        sender_flowMap.erase(snd_info.flowid);
    }else{
        sender_flowMap[snd_info.flowid]=snd_info;
    }
    if(accumlength>=TIMELYseg){
        accumlength=0;
        simtime_t nxtSendTime = simtime_t((TIMELYseg+segcount*58)*8/currentRate) + simTime();
        segcount=0;
        //58=20(IP)+14(EthernetMac)+8(EthernetPhy)+4(EthernetFcs)+12(interframe gap,IFG)
        EV<<"segment finish, nxtSendTime = "<<nxtSendTime<<endl;
        scheduleAt(nxtSendTime,senddata);
    }else{
        scheduleAt(simTime(),senddata);
    }

    sendDown(packet);
}

// Record the packet from udp to transmit it to the dest


void TIMELY::processLowerpck(Packet *pck)
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

void TIMELY::receive_data(Packet *pck)
{
    sendUp(pck);
}

void TIMELY::receive_ack(Packet *pck)
{
    lastRTT = currentRTT;
    currentRTT = pck->getTimestamp();
    EV << "RTT is " << currentRTT << ", lastRTT is " << lastRTT <<endl;
    double new_rtt_diff = currentRTT.dbl() - lastRTT.dbl();
    rtt_diff = (1 - alpha) * rtt_diff + alpha * new_rtt_diff;
    double gradient = rtt_diff / minRTT.dbl();

    EV << "currentRate = " << currentRate << endl;
    EV << "new_rtt_diff = " << new_rtt_diff << ", true rtt_diff = " << rtt_diff  << ", gradient = " << gradient << endl;

    if(lastRTT!=0){//第一个RTT还没有梯度
        if(currentRTT < Tlow){
            EV<<"currentRTT < Tlow"<<endl;
            currentRate = currentRate + Rai;
        }else if (currentRTT > Thigh){
            EV<<"currentRTT > Thigh"<<endl;
            currentRate = currentRate * (1 - beta * (1 - Thigh/currentRTT));
        }else if(gradient <= 0){
            EV<<"currentRTT between Tlow and Thigh, gradient <= 0"<<endl;
            if(Number >= 5)
                currentRate += 5 * Rai;
            else{
                currentRate += Rai;
                Number++;
            }
        }else{
            EV<<"currentRTT between Tlow and Thigh, gradient > 0"<<endl;
            currentRate = currentRate * (1 - beta * (gradient));
            Number = 1;
        }

    }

    EV << "currentRate = " << currentRate << endl;

    delete pck;
}

void TIMELY::sendDown(Packet *pck)
{
    EV << "TIMELY, sendDown " <<pck->getFullName()<<endl;
    send(pck,lowerOutGate);
}

void TIMELY::sendUp(Packet *pck)
{
    EV<<"TIMELY, sendup!"<<endl;
    send(pck,upperOutGate);
}


void TIMELY::refreshDisplay() const
{

}


void TIMELY::finish()
{

}

}
