/*
 * PCN.cc
 *
 *  Created on: 20200605
 *      Author: zhouyukun
 */
#include "PCN.h"

namespace inet {

Define_Module(PCN);

void PCN::initialize()
{
    //gates
    lowerOutGate = gate("lowerOut");
    lowerInGate = gate("lowerIn");
    upperOutGate = gate("upperOut");
    upperInGate = gate("upperIn");
    // configuration
    stopTime = par("stopTime");
    activate = par("activate");
    omega_min = par("omega_min");
    omega_max = par("omega_max");
    linkspeed = par("linkspeed");
    min_cnp_interval = par("min_cnp_interval");
    max_pck_size = par("max_pck_size");

    senddata = new TimerMsg("senddata");
    senddata->setKind(SENDDATA);

    registerService(Protocol::udp, gate("upperIn"), gate("upperOut"));
    registerProtocol(Protocol::udp, gate("lowerOut"), gate("lowerIn"));
}

void PCN::handleMessage(cMessage *msg)
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

void PCN::handleSelfMessage(cMessage *pck)
{
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

// Record the packet from udp to transmit it to the dest
void PCN::processUpperpck(Packet *pck)
{
    if (string(pck->getFullName()).find("Data") != string::npos&&activate==true){
        sender_flowinfo snd_info;
        for (auto& region : pck->peekData()->getAllTags<HiTag>()){
            snd_info.flowid = region.getTag()->getFlowId();
            snd_info.cretime = region.getTag()->getCreationtime();
            snd_info.priority = region.getTag()->getPriority();
            EV << "store new flow, id = "<<snd_info.flowid<<
                    ", creationtime = "<<snd_info.cretime<<endl;
        }
        auto addressReq = pck->addTagIfAbsent<L3AddressReq>();
        srcAddr = addressReq->getSrcAddress();
        L3Address destAddr = addressReq->getDestAddress();

        auto udpHeader = pck->removeAtFront<UdpHeader>();

        snd_info.remainLength = pck->getByteLength();
        snd_info.destAddr = destAddr;
        snd_info.srcPort = udpHeader->getSrcPort();
        snd_info.destPort = udpHeader->getDestPort();
        snd_info.pckseq = 0;
        snd_info.crcMode = udpHeader->getCrcMode();
        snd_info.crc = udpHeader->getCrc();
        snd_info.currentRate = linkspeed;
        snd_info.omega = omega_min;
        if(sender_flowMap.empty()){
            sender_flowMap[snd_info.flowid]=snd_info;
            iter=sender_flowMap.begin();//iter needs to be assigned after snd_info is inserted
        }else{
            sender_flowMap[snd_info.flowid]=snd_info;
        }

        // send packet timer
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

void PCN::send_data()
{
    sender_flowinfo snd_info=iter->second;
    std::ostringstream str;
    str << packetName << "-" <<snd_info.flowid<< "-" <<snd_info.pckseq;
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
    EV<<"send_data(), flowid = "<<snd_info.flowid<<", pcklength = "<<pcklength<<endl;
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

    if (snd_info.remainLength == 0){
        sender_flowMap.erase(snd_info.flowid);
        iter++;
    }else{
        sender_flowMap[snd_info.flowid]=snd_info;
    }
    simtime_t nxtSendTime = simtime_t((packet->getByteLength()+58)*8/snd_info.currentRate) + simTime();
    //58=20(IP)+14(EthernetMac)+8(EthernetPhy)+4(EthernetFcs)+12(interframe gap,IFG)
    scheduleAt(nxtSendTime,senddata);
    sendDown(packet);
}

void PCN::processLowerpck(Packet *pck)
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

void PCN::receive_data(Packet *pck)
{
    auto l3AddressInd = pck->getTag<L3AddressInd>();
    auto srcAddr = l3AddressInd->getSrcAddress();
    auto destAddr = l3AddressInd->getDestAddress();
    auto ecn = pck->addTagIfAbsent<EcnInd>()->getExplicitCongestionNotification();
    uint flowid;
    int pckseq;
    simtime_t cretime;
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        flowid = region.getTag()->getFlowId();
        pckseq = region.getTag()->getPacketId();
        cretime = region.getTag()->getCreationtime();
    }
    receiver_flowinfo rcvinfo;

    if(receiver_flowMap.find(flowid) == receiver_flowMap.end())
    {
        rcvinfo.lastCnpTime = simTime();
        rcvinfo.nowHTT = simTime() - cretime;
        rcvinfo.ecnPakcetReceived = false;
        rcvinfo.Sender_srcAddr = srcAddr;
        rcvinfo.Sender_destAddr = destAddr;
        rcvinfo.recNum = 1;
        rcvinfo.recData = pck->getBitLength();
        rcvinfo.cnp_interval_num=1;
        if(ecn == 3){
            rcvinfo.ecnNum=1;
        }else{
            rcvinfo.ecnNum=0;
        }
        receiver_flowMap[flowid] = rcvinfo;
    }
    else
    {
        rcvinfo = receiver_flowMap.find(flowid)->second;
        rcvinfo.recNum++;
        rcvinfo.recData += pck->getBitLength();
        if(ecn == 3)
            rcvinfo.ecnNum++;
        receiver_flowMap[flowid] = rcvinfo;
    }

    simtime_t intervalTime = simTime() - rcvinfo.lastCnpTime;

    EV<<"Flowid = "<<flowid<<", receive number = "<<rcvinfo.recNum<<", receive data = "<<rcvinfo.recData<<", receive ecn number = "<<rcvinfo.ecnNum<<", intervalTime = "<<intervalTime.dbl()<<endl;

    if(intervalTime.dbl() >= rcvinfo.cnp_interval_num*min_cnp_interval.dbl()){
        double tempRecRate = 0;
        if(rcvinfo.recNum > 0){
            double ECN_rate = 0.00;
            ECN_rate = (double)rcvinfo.ecnNum / (double)rcvinfo.recNum;
            EV<<"ECN rate = "<<ECN_rate<<endl;
            //ECN_rate setting
            if(ECN_rate > 0.95){
                if(rcvinfo.recNum > 1){
                    tempRecRate = rcvinfo.recData / intervalTime.dbl();
                    rcvinfo.ecnPakcetReceived = true;
                    rcvinfo.recRate = tempRecRate;
                    receiver_flowMap[flowid] = rcvinfo;
                }
                else{
                    tempRecRate = rcvinfo.recData / rcvinfo.nowHTT.dbl();
                    rcvinfo.ecnPakcetReceived = true;
                    rcvinfo.recRate = tempRecRate;
                    receiver_flowMap[flowid] = rcvinfo;
                }
            }
            else{
                rcvinfo.ecnPakcetReceived = false;
                rcvinfo.recRate = 0;
                receiver_flowMap[flowid] = rcvinfo;
            }
            EV<<"Flowid = "<<flowid<<", receive number = "<<rcvinfo.recNum<<", receive data = "
                    <<rcvinfo.recData<<", receive ecn number = "<<rcvinfo.ecnNum<<", intervalTime = "<<intervalTime.dbl()
                    <<", receive rate = "<<rcvinfo.recRate<<endl;
            send_cnp(flowid);
        }else{
            rcvinfo.cnp_interval_num++;
        }
        rcvinfo.recNum = 0;
        rcvinfo.recData = 0;
        rcvinfo.ecnNum = 0;
        receiver_flowMap[flowid] = rcvinfo;
    }
    send_cnp(flowid);

    EV<<"receive packet, sequence number = "<<pckseq<<", ecn = "<<ecn<<endl;
    sendUp(pck);
}

void PCN::send_cnp(uint32_t flowid)
{
    receiver_flowinfo rcvinfo = receiver_flowMap.find(flowid)->second;
    Packet *cnp = new Packet("CNP");

    const auto& recRateChunk = makeShared<CNP>();
    recRateChunk->setRecRate(rcvinfo.recRate);
    cnp->insertAtFront(recRateChunk);
    EV_INFO << "recRate = "<< rcvinfo.recRate <<endl;

    if(rcvinfo.ecnPakcetReceived == true){
        cnp->addTagIfAbsent<EcnReq>()->setExplicitCongestionNotification(1);
    }
    else{
        cnp->addTagIfAbsent<EcnReq>()->setExplicitCongestionNotification(0);
    }

    cnp->addTagIfAbsent<L3AddressReq>()->setDestAddress(rcvinfo.Sender_srcAddr);
    cnp->addTagIfAbsent<L3AddressReq>()->setSrcAddress(rcvinfo.Sender_destAddr);
    cnp->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    cnp->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::udp);
    EV_INFO << "Flowid = "<< flowid << ", Sending CNP packet to "<< rcvinfo.Sender_srcAddr << ", ECN mark = "<< rcvinfo.ecnPakcetReceived <<", cnp interval = "<<(simTime()-rcvinfo.lastCnpTime).dbl()<<endl;
    rcvinfo.lastCnpTime = simTime();
    sendDown(cnp);
    receiver_flowMap[flowid] = rcvinfo;
}

void PCN::receive_cnp(Packet *pck)
{
    uint32_t flowid;
    L3Address destAddr = pck->addTagIfAbsent<L3AddressInd>()->getSrcAddress();
    auto ecn = pck->addTagIfAbsent<EcnInd>()->getExplicitCongestionNotification();
    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        flowid = region.getTag()->getFlowId();
    }

    EV_INFO << "Receiving CNP packet from dest: "<< destAddr << ", ECN mark "<< ecn <<endl;

    if(sender_flowMap.find(flowid)!=sender_flowMap.end()){
        EV<<"flow exists"<<endl;
        sender_flowinfo sndinfo = sender_flowMap.find(flowid)->second;
        //decrease rate
        if(ecn == 1){
            auto recRateChunk = pck->peekAtFront<CNP>();

            EV<<"recRate = "<<recRateChunk->getRecRate()<<endl;

            EV<<"before decreasing, the current rate = "<<sndinfo.currentRate<<endl;

            auto recRate = recRateChunk->getRecRate();

            double tempRecRate = recRate * (1 - omega_min);
            if(sndinfo.currentRate > tempRecRate){

                sndinfo.currentRate = tempRecRate;
            }
            sndinfo.omega = omega_min;
            EV<<"after decreasing, the current rate = "<<sndinfo.currentRate<<
                            ", target rate = "<<recRate * (1 - omega_min)<<endl;
        }
        //increase rate
        else if(ecn == 0){
            sndinfo.currentRate = sndinfo.currentRate * (1 - sndinfo.omega) + linkspeed * sndinfo.omega;
            sndinfo.omega = sndinfo.omega * (1 - sndinfo.omega) + omega_max * sndinfo.omega;
        }
        sender_flowMap[flowid] = sndinfo;
    }
    delete pck;
}

void PCN::sendDown(Packet *pck)
{
    EV << "PCN, sendDown " <<pck->getFullName()<<endl;
    send(pck,lowerOutGate);
}

void PCN::sendUp(Packet *pck)
{
    EV<<"PCN, sendup!"<<endl;
    send(pck,upperOutGate);
}

void PCN::refreshDisplay() const
{

}


void PCN::finish()
{

}

}
