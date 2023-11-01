/*
 * BFC.cc
 *
 *  Created on: 2023��5��30��
 *      Author: ergeng2001
 */



#include "BCN.h"
#include "inet/HiNA/Messages/HiTag/HiTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/common/clock/ClockUserModuleMixin.h"

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"
#include "inet/applications/tcpapp/GenericAppMsg_m.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>

namespace inet {

Define_Module(BCN);

void BCN::initialize(int stage)
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

void BCN::handleMessage(cMessage *msg)
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

void BCN::handleSelfMessage(cMessage *pck)
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
void BCN::processUpperpck(Packet *pck)
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
           // std::cout<<"BCN flowid = "<< flowid<<endl;
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

void BCN::send_data()
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
    scheduleAt(nxtSendTime,senddata);//�ȵ�����send data������flowMap��û�и���
    EV<<"BCN send "<<packet << "simTime = "<<simTime()<<" ,set last creation time ="<<last_creation_time <<endl;
    sendDown(packet);
}

void BCN::processLowerpck(Packet *pck)
{
    if (std::string(pck->getFullName()).find("routerCNP") != std::string::npos)//���յ�CNP��
    {
        EV<<"receive router cnp."<<endl;
        receive_bfccnp(pck);
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

void BCN::receive_data(Packet *pck)//���� ����CNP��
{
    auto l3AddressInd = pck->getTag<L3AddressInd>();
    auto rcv_srcAddr = l3AddressInd->getSrcAddress();
    auto rcv_destAddr = l3AddressInd->getDestAddress();//ԴIP��Ŀ��IP��ַ
    sendUp(pck);
}

void BCN::receive_bfccnp(Packet *pck)
{//ÿ����ֻ�ᴥ��һ�ν��ٺ����٣�
    //�ڽ��ٽ׶Σ������ʽ���alpha��ֵ�����ʣ�
    //����ֱ�ӽ��͵�1/2�����ʣ�
    //�����ٽ׶Σ������ٻָ���ԭ����Ŀ�����ʣ�
    //���������ÿ�����Ľ��ٺ����ٹ���
    //���ٰ�ֻ�ήһ�Σ����ٰ�Ҳֻ����һ�Σ�

    //���ʡȥ��DCQCN�ĺܶ�����ϸ�ڣ��ڷ������ݰ���
    //�������ٹ��ܣ�����Ѿ�

    //ÿ������¼�Ѿ����͵��ֽ�����
    //�յ������ź�ʱ�����remainLength < flowsize * 1/2�������ʵ�����ԭ���� 1/4; ���remainLength > flowsize * 1/2�������ʵ�����ԭ���� 1/2
    //�յ������ź�ʱ�����remainLength < flowsize * 1/2�������ʵ����� Ŀ�����ʣ���һ�ν���֮ǰ�����ʣ�* 3/2�����remainLength > flowsize * 1/2�������ʵ��� Ŀ������;
    uint32_t flowid;//
    bool isDeceleration;
    bool isSpeedup;

    for (auto& region : pck->peekData()->getAllTags<HiTag>()){
        flowid = region.getTag()->getFlowId();
        isDeceleration = region.getTag()->isDeceleration();
        isSpeedup = region.getTag()->isSpeedup();
    }
    EV<<"router CNP packet = "<< pck<<endl;

    EV<<"Find flowid = "<< flowid <<endl;

    uint64_t remainlength;

    if(sender_flowMap.find(flowid)!=sender_flowMap.end()){//��map�У��ҵ���ǰ����flowinfo


    remainlength = sender_flowMap[flowid].remainLength;

    EV<<"remain length = "<<remainlength <<endl;

    if(remainlength != 0){

        sender_flowinfo sndinfo = sender_flowMap.find(flowid)->second;//ȡ����flowid->second

    if(isDeceleration){//����

    //ԭ����cnp���ٷ���
        // cut rate
        EV<<"Now receive deceleration"<<endl;
        sndinfo.targetRate = sndinfo.currentRate;
        //��ȡ����ǰ�Ѿ��������ֽ���
        if(sndinfo.remainLength < sndinfo.flowsize * 1/2){
            sndinfo.currentRate = sndinfo.currentRate * 0.95;
        }else{
            sndinfo.currentRate = sndinfo.currentRate * 0.99;
        }
        receivecnpdel++;
        EV<<pck<<"after cutting, the current rate = "<<sndinfo.currentRate<<
                ", target rate = "<<sndinfo.targetRate<<endl;

    }

   else if(isSpeedup){//���� ����ԭ���ı��ص����ٹ��̣� �ڸ��ݽ��յ���speedup���������ٹ��̣�

       EV<<"Now receive speedup."<<endl;
       //����
       if(sndinfo.remainLength < sndinfo.flowsize * 1/2){
           receivecnpspeup++;
            sndinfo.currentRate = sndinfo.targetRate *1;
       }else{
           receivecnpspeorigin++;
            sndinfo.currentRate = sndinfo.targetRate * 1.2;
       }

       EV<< pck <<"after increasing, the current rate = "<<sndinfo.currentRate<<
                    ", target rate = "<<sndinfo.targetRate<<endl;

   }
    sender_flowMap[flowid] = sndinfo;

    }
  }

    delete pck;
}


void BCN::sendDown(Packet *pck)
{
    EV << "sendDown " <<pck->getFullName()<<"simTime = "<<simTime()<<endl;
    send(pck,lowerOutGate);
}

void BCN::sendUp(Packet *pck)
{
    EV<<"sendup"<<endl;
    send(pck,upperOutGate);
}

void BCN::refreshDisplay() const
{

}

void BCN::finish()
{
    recordScalar("receive cnp delerlation",receivecnpdel);
    recordScalar("receive cnp speed origin",receivecnpspeorigin);
    recordScalar("receive cnp speed up",receivecnpspeup);
}

} // namespace inet


