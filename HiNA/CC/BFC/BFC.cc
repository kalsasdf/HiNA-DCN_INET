/*
 * BFC.cc
 *
 *  Created on: 2023年6月27日
 *      Author: ergeng2001
 */



#include "BFC.h"

namespace inet {

Define_Module(BFC);

void BFC::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL){

        //for statistic
        WATCH(receivecnpdel);
        WATCH(receivecnpspeorigin);
        WATCH(receivecnpspeup);

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
        Rhai = par("Rhai");//4e8bps
//        maxTxRate = par("maxTxRate");
        max_pck_size=par("max_pck_size");
        senddata = new TimerMsg("senddata");
        senddata->setKind(SENDDATA);


    }else if (stage == INITSTAGE_TRANSPORT_LAYER) {
        registerService(Protocol::udp, gate("upperIn"), gate("upperOut"));
        registerProtocol(Protocol::udp, gate("lowerOut"), gate("lowerIn"));
    }
}

void BFC::handleMessage(cMessage *msg)
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

void BFC::handleSelfMessage(cMessage *pck)
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

// Record the packet from app to transmit it to the dest
void BFC::processUpperpck(Packet *pck)
{
    if (string(pck->getFullName()).find("Data") != string::npos&&activate==true){
        int flowid;
        simtime_t cretime;
        int priority;
        int queueid;

        //for recorder flowSize;
        uint64_t flowsize;

        for (auto& region : pck->peekData()->getAllTags<HiTag>()){

            flowid = region.getTag()->getFlowId();
           // std::cout<<"BFC flowid = "<< flowid<<endl;
            cretime = region.getTag()->getCreationtime();
            priority = region.getTag()->getPriority();
            queueid = region.getTag()->getQueueID();
            flowsize = region.getTag()->getFlowSize();
        }
        if(sender_flowMap.find(flowid)!=sender_flowMap.end()){ // flowid in map
            sender_flowMap[flowid].remainLength+=pck->getByteLength();
        }else{
            sender_flowinfo snd_info;
            snd_info.flowid = flowid;

            snd_info.cretime = cretime;
//            snd_info.cretime = simTime();
            snd_info.queueid = queueid;
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
            snd_info.flowsize = flowsize;
            snd_info.pckseq = 0;
            snd_info.crcMode = udpHeader->getCrcMode();
            snd_info.crc = udpHeader->getCrc();
            snd_info.currentRate = linkspeed*initialrate;
            snd_info.targetRate = snd_info.currentRate;

            //
            snd_info.flow_maxTxRate =  linkspeed*initialrate*2;

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

void BFC::send_data()
{
    //int id=36;
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
    const auto payload = makeShared<ByteCountChunk>(B(pcklength));
    payload->enableImplicitChunkSerialization = true;
    auto tag = payload->addTag<HiTag>();
    tag->setFlowId(snd_info.flowid);
    tag->setQueueID(snd_info.queueid);

    tag->setPriority(snd_info.priority);
//    tag->setCreationtime(snd_info.cretime);
    if(last_flowid != snd_info.flowid){
        last_creation_time = simTime();
        last_flowid = snd_info.flowid;
    }

    tag->setCreationtime(last_creation_time);


    if(snd_info.remainLength == 0){
        tag->setIsLastPck(true);
    }

    tag->setPacketId(snd_info.pckseq);
    //for record flowsize
    tag->setFlowSize(snd_info.flowsize);
    snd_info.pckseq += 1;

    packet->insertAtBack(payload);


    auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
    addressReq->setSrcAddress(srcAddr);
    addressReq->setDestAddress(snd_info.destAddr);

    // insert udpHeader, set source and destination port
    const Protocol *l3Protocol = &Protocol::ipv4;
    auto udpHeader = makeShared<UdpHeader>();
    udpHeader->enableImplicitChunkSerialization = true;
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
        iter++;
        sender_flowMap.erase(snd_info.flowid);
    }else{
        sender_flowMap[snd_info.flowid] =snd_info;
    }
    EV<<"snd_info.currentRate = "<< snd_info.currentRate <<endl;
    simtime_t d = simtime_t((packet->getByteLength()+58)*8/snd_info.currentRate);EV<<"send interval = "<<d<<endl;
    simtime_t nxtSendTime = simtime_t((packet->getByteLength()+58)*8/snd_info.currentRate) + simTime();
    //58=20(IP)+14(EthernetMac)+8(EthernetPhy)+4(EthernetFcs)+12(interframe gap,IFG)
    scheduleAt(nxtSendTime,senddata);//先调用了send data，但是flowMap还没有更新
    EV<<"BFC send "<<packet << "simTime = "<<simTime()<<" ,set last creation time ="<<last_creation_time <<endl;
    sendDown(packet);
}

void BFC::processLowerpck(Packet *pck)
{
    if (!strcmp(pck->getFullName(),"routerCNP"))//接收到CNP包
    {
        EV<<"receive router cnp"<<endl;

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

void BFC::receive_data(Packet *pck)//触发 发送CNP包
{
    auto l3AddressInd = pck->getTag<L3AddressInd>();
    auto rcv_srcAddr = l3AddressInd->getSrcAddress();
    auto rcv_destAddr = l3AddressInd->getDestAddress();//源IP和目的IP地址
    sendUp(pck);
}



void BFC::sendDown(Packet *pck)
{
    EV << "sendDown " <<pck->getFullName()<<"simTime = "<<simTime()<<endl;
    send(pck,lowerOutGate);
}

void BFC::sendUp(Packet *pck)
{
    EV<<"sendup"<<endl;
    send(pck,upperOutGate);
}

void BFC::refreshDisplay() const
{

}

void BFC::finish()
{
    recordScalar("receive cnp delerlation",receivecnpdel);
    recordScalar("receive cnp speed origin",receivecnpspeorigin);
    recordScalar("receive cnp speed up",receivecnpspeup);
}

} // namespace inet




