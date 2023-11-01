//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "BCNqueue.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/PacketEventTag.h"
#include "inet/common/Simsignals.h"
#include "inet/common/TimeTag.h"
#include "inet/queueing/function/PacketComparatorFunction.h"
#include "inet/queueing/function/PacketDropperFunction.h"
#include "inet/common/packet/chunk/Chunk.h"
#include "inet/common/packet/Packet.h"

#include <vector>
#include <algorithm>

#include "inet/networklayer/common/NetworkInterface.h"

#include "inet/linklayer/base/MacProtocolBase.h"
#include <algorithm>
#include "inet/HiNA/Messages/BFCHeader/BFCHeader_m.h"
#include "inet/HiNA/Messages/BFCHeader/BFCHeaderSerializer.h"
#include "inet/HiNA/Messages/BFCTag/isPause_m.h"

#include "inet/networklayer/common/NetworkInterface.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"


namespace inet {

Define_Module(BCNqueue);

simsignal_t BCNqueue::bfcPausedSignal =
        cComponent::registerSignal("bfcPaused");
simsignal_t BCNqueue::bfcResumeSignal =
        cComponent::registerSignal("bfcResume");

simsignal_t BCNqueue::bfcDecelerationSignal =
        cComponent::registerSignal("bfcDeceleration");
simsignal_t BCNqueue::bfcSpeedupSignal =
        cComponent::registerSignal("bfcSpeedup");

b BCNqueue::sharedBuffer[100][100]={};


void BCNqueue::initialize(int stage)
{
    PacketQueueBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {

        WATCH(sendcnpdel);
        WATCH(sendcnpspe);

        queue.setName("storage");
        producer = findConnectedModule<IActivePacketSource>(inputGate);
        collector = findConnectedModule<IActivePacketSink>(outputGate);
        packetCapacity = par("packetCapacity");
        EV<<"packetCapacity = "<<packetCapacity  <<endl;//4000个包
        dataCapacity = b(par("dataCapacity"));
        EV<<"dataCapacity = "<<dataCapacity <<endl;//50MB
        switchid=findContainingNode(this)->getId();EV<<"switchid = "<<switchid<<endl;
//        EV<<"switch = "<<findContainingNode(this)<<endl;
//        EV<<"switchid = "<<switchid<<endl;
        //找到包含当前queue的模块,即eth模块，识别出该eth模块的端口号；作为switchid
       // priority = par("priority"); //set in .ned is -1
        numb = par("numb");//队列id
        sharedBuffer[switchid][numb] = b(par("sharedBuffer"));
        EV<<"sharedBuffer[switchid][numb] = "<<sharedBuffer[switchid][numb] <<endl;//1.5MB
        headroom = b(par("headroom"));//179200b == 22400B
        EV<<"headroom ="<<headroom <<endl;
        useBfc = par("useBfc");
//        useBfc = true;
        //std::cout<<"useBfc = "<<useBfc<<endl;
        XON = par("XON");
        XOFF = par("XOFF");
        Kmax=par("Kmax");
        Kmin=par("Kmin");
        Pmax=par("Pmax");
        useEcn=par("useEcn");
        alpha=par("alpha");
        //for sharedBuffer
        queuelengthVector.setName("queuelength (bit)");
        sharedBufferVector.setName("sharedbuffer (bit)");
        //for sharedBuffer
        count = -1;
        packetComparatorFunction = createComparatorFunction(par("comparatorClass"));
        if (packetComparatorFunction != nullptr)
            queue.setup(packetComparatorFunction);
        packetDropperFunction = createDropperFunction(par("dropperClass"));
    }
    else if (stage == INITSTAGE_QUEUEING) {
        checkPacketOperationSupport(inputGate);
        checkPacketOperationSupport(outputGate);
        if (producer != nullptr)
            producer->handleCanPushPacketChanged(inputGate->getPathStartGate());
    }
    else if (stage == INITSTAGE_LAST)
        updateDisplayString();
}

IPacketDropperFunction *BCNqueue::createDropperFunction(const char *dropperClass) const
{
    if (strlen(dropperClass) == 0)
        return nullptr;
    else
        return check_and_cast<IPacketDropperFunction *>(createOne(dropperClass));
}

IPacketComparatorFunction *BCNqueue::createComparatorFunction(const char *comparatorClass) const
{
    if (strlen(comparatorClass) == 0)
        return nullptr;
    else
        return check_and_cast<IPacketComparatorFunction *>(createOne(comparatorClass));
}

bool BCNqueue::isOverloaded() const
{
    return (packetCapacity != -1 && getNumPackets() > packetCapacity) ||
           (dataCapacity != b(-1) && getTotalLength() > dataCapacity);
}

int BCNqueue::getNumPackets() const
{
    return queue.getLength();
}

Packet *BCNqueue::getPacket(int index) const
{
    if (index < 0 || index >= queue.getLength())
        throw cRuntimeError("index %i out of range", index);
    return check_and_cast<Packet *>(queue.get(index));
}

void BCNqueue::handleMessage(cMessage *message)
{
    auto packet = check_and_cast<Packet *>(message);
    if(BufferManagement(message))
    {
        pushPacket(packet, packet->getArrivalGate());//这里的阈值设的太大了；导致先丢包了，但是还没感知到队列排队超过阈值
    }
    else{
        dropPacket(packet, QUEUE_OVERFLOW);
    }
}

void BCNqueue::pushPacket(Packet *packet, cGate *gate)
{

    Enter_Method("pushPacket");
    take(packet);
    cNamedObject packetPushStartedDetails("atomicOperationStarted");
    emit(packetPushStartedSignal, packet, &packetPushStartedDetails);
    EV_INFO << "Pushing packet" << EV_FIELD(packet) << EV_ENDL;
    queue.insert(packet);

    int iface = packet->addTagIfAbsent<InterfaceInd>()->getInterfaceId(); //iface is the packet form port;not the now port;
    if (std::string(packet->getFullName()).find("BCN")!= std::string::npos){
    if(useBfc){
        if(queue.getByteLength()>=XOFF||(queue.getBitLength()>dataCapacity.get()&&sharedBuffer[switchid][numb]<headroom)){
            EV<<"###########When push packet, queuelength overload! ############"<<endl;
            cModule *radioModule = getParentModule()->getParentModule(); //BCNqueue -> BCNQueue -> eth
            NetworkInterface* eth_Interface = check_and_cast<NetworkInterface*>(radioModule);

            EV<<"Packet push in eth = "<<radioModule<<endl;
            EV<<"Packet push in switch = "<<radioModule->getParentModule()<<endl;

            if(queue.getByteLength()>=XOFF){
                EV<<"queue.getByteLength()>=XOFF: "<<queue.getByteLength() << " >= "<<XOFF<<endl;
            }else{
                EV<<"queue.getBitLength()>dataCapacity.get()&&sharedBuffer[switchid][numb]<headroom"<<endl;
                EV<<"queue.getBitLength()>dataCapacity.get(): "<<queue.getBitLength()<<" > "<<dataCapacity.get()<<endl;
                EV<<"sharedBuffer[switchid][numb]<headroom: "<< sharedBuffer[switchid][numb] << " < "<< headroom<<endl;
            }

            //get数据包的BFCHeader头部中的upstreamQ，用于存入pause signal packet中；
            b offset(0);
            auto ethHeader = packet->peekDataAt<EthernetMacHeader>(offset);
            offset += ethHeader->getChunkLength();
            auto bfcHeader = packet->peekDataAt<BFCHeader>(offset);
            int upstreamQueueID = bfcHeader->getUpstreamQueueID();

            //get数据包的ip_hdr的源地址和目的地址，用于准确发送CNP包到指定的主机地址；
            offset += bfcHeader->getChunkLength();
            auto ip_hdr = packet->peekDataAt<Ipv4Header>(offset);
            L3Address destAddr = ip_hdr->getDestinationAddress();
            L3Address srcAddr = ip_hdr->getSourceAddress();

            //get数据包的flowid，用于CNP准确降速某条流量；
            uint32_t flowid;
            for (auto& region : packet->peekData()->getAllTags<HiTag>()){
                flowid = region.getTag()->getFlowId();
            }

            EV<< "isPausedFlow[i] = "<< endl;
            for(int i = 0;i < isPausedFlow.size();i++){
                EV<< isPausedFlow[i] << " ";
            }
            EV<<endl;

            if(std::find(isPausedFlow.begin(),isPausedFlow.end(),flowid) !=isPausedFlow.end() ){//flow in vector
                //已经存在，说明该条流已经发送了pause帧和CNP降速包
            }else{
                //该packet是这条flow中的第一个触发cnp降速包的packet，因此，给这个packet打上ispause标记
                auto PauseTag = packet -> addTag<isPause>();
                PauseTag->setBfcisPause(true);

                isPausedFlow.push_back(flowid);//插入flowid
                EV<<"After insert flowid = "<<endl;
                for(int i = 0;i < isPausedFlow.size();i++){
                    EV<< isPausedFlow[i] << " ";
                }
                EV<<endl;

                //发送CNP通知信号，给本地的mac
                if(std::string(packet->getFullName()).find("BCN")!= std::string::npos){//只有BCN具有发送降速包的功能
                    auto pck = new Packet("deceleration");
                    auto newtag = pck->addTagIfAbsent<HiTag>();
                    newtag->setOp(ETHERNET_BFC_DECELERATION);

                    newtag->setIsDeceleration(true);
                    newtag->setLocal_interfaceId(eth_Interface->getInterfaceId());
                    newtag->setFlowId(flowid);
                    newtag->setSrcAddr(srcAddr);
                    newtag->setDestAddr(destAddr);

                    emit(bfcDecelerationSignal,pck);
                    sendcnpdel++;

                    delete pck;
                    EV<<"BCNqueue send deceleration cnp to flow--"<<flowid<<endl;
                }//for BCN send deceleration packet
            }//for BCN first isPaused Packet. send deceleration packet

            //检查上游交换机 multimap ispaused<eth,upstreamQid>，是否暂停
            if(ispaused.find(iface)!= ispaused.end()){//eth in multimap ispaused<eth,upstreamQid>
                std::pair<iter_ef,iter_ef> itef = ispaused.equal_range(iface);//用指针类型的pair存储所有键值为iface的所有项；
                bool isin = false;//用于记录<eth,upstreamQid>是否在ispaused里面；
                while(itef.first!=itef.second){
                    if(upstreamQueueID == itef.first->second){// <eth,upstreamQid> 存在于ispaused
                        isin = true;
                    }
                    itef.first++;
                }
                if(!isin){//multimap中不存在<eth,upstreamQid>。进行以下操作：①插入eth_upstream;②触发发送暂停帧

                    ispaused.insert(std::pair<int,int>(iface,upstreamQueueID));
                    auto pck = new Packet("pause");
                    auto newtag=pck->addTagIfAbsent<HiTag>();
                    newtag->setOp(ETHERNET_BFC_PAUSE);
                    newtag->setQueueID(upstreamQueueID);
                    newtag->setInterfaceId(iface);
                    emit(bfcPausedSignal,pck);
                    delete pck;
                    EV<<"Send Paused packet."<<endl;
                    EV<<"Insert multimap <"<< iface << ","<<upstreamQueueID<<" >"<< endl;
                }//eth in multimap，upstreamQid not in multimap

           }else{//eth not in map
                if(ispaused.empty()){//<eth,upstreamQid>插入，并触发发送暂停帧
                    ispaused.insert(std::pair<int,int>(iface,upstreamQueueID));
                    iter_ef efptr;
                    efptr = ispaused.begin();
                }
                else{
                     ispaused.insert(std::pair<int,int>(iface,upstreamQueueID));
                    }
                    //发送暂停帧
                    auto pck = new Packet("pause");
                    auto newtag=pck->addTagIfAbsent<HiTag>();
                    newtag->setOp(ETHERNET_BFC_PAUSE);
                    newtag->setQueueID(upstreamQueueID);
                    newtag->setInterfaceId(iface);
                    emit(bfcPausedSignal,pck);
                    delete pck;
                    EV<<"Send Paused packet."<<endl;
                    EV<<"eth not in map. Insert multimap < "<<iface<<", "<<upstreamQueueID<< ">"<<endl;
            }//for eth not in map. send pause signal
        }//for queuelength is overload!
    }//for useBfc
  }//for BCN packet

    if (collector != nullptr && getNumPackets() != 0)
        collector->handleCanPullPacketChanged(outputGate->getPathEndGate());
    cNamedObject packetPushEndedDetails("atomicOperationEnded");
    emit(packetPushEndedSignal, nullptr, &packetPushEndedDetails);
    updateDisplayString();
}


Packet *BCNqueue::pullPacket(cGate *gate)
{

    Enter_Method("pullPacket");
    auto packet = check_and_cast<Packet *>(queue.front());
    int iface = packet->addTagIfAbsent<InterfaceInd>()->getInterfaceId();

    if (std::string(packet->getFullName()).find("BCN") != std::string::npos){

        //get数据包的BFCHeader头部中的upstreamQid，用于存入resume signal packet中；
        b offset(0);
        b offset2(0);
        auto ethHeader = packet->peekDataAt<EthernetMacHeader>(offset);
        offset += ethHeader->getChunkLength();
        offset2 +=ethHeader->getChunkLength();//记录mac头部的chunk大小
        auto bfcHeader = packet->peekDataAt<BFCHeader>(offset);
        int upstreamQueueID = bfcHeader->getUpstreamQueueID();

        //获取packet的flowid
        uint32_t flowid;
        for (auto& region : packet->peekData()->getAllTags<HiTag>()){
            flowid = region.getTag()->getFlowId();
        }

        //当正在传输isPause数据包时，立即触发恢复帧，向该包所在的上游指定端口和指定队列；
        auto pisPaused = packet ->findTag<isPause>();
        if(pisPaused != nullptr){
        if(pisPaused->getBfcisPause()){
            EV<<"###########Now this BCNqueue ispaused packet is pulled, Send resume !!#############"<<endl;
            cModule *radioModule = getParentModule()->getParentModule(); //BCNqueue -> BCNQueue -> eth
            NetworkInterface* eth_Interface = check_and_cast<NetworkInterface*>(radioModule);
            EV<<"Pull packet from eth = "<<radioModule<<endl;
            EV<<"Pull packet from switch = "<<radioModule->getParentModule()<<endl;// //BCNqueue -> BCNQueue -> eth ->switch

            //输出提示，当前的队列长度信息
            EV<<"queue.getByteLength() = "<<queue.getByteLength() <<endl;
            EV<< "isPausedFlow[i] = "<<endl;
            for(int i = 0;i < isPausedFlow.size();i++){
                EV<< isPausedFlow[i] << " ";
            }
            EV<<endl;

            if(std::find(isPausedFlow.begin(),isPausedFlow.end(),flowid) !=isPausedFlow.end() ){//flow in vector
                //删除flowid，发送升速cnp
                auto iter = std::remove(isPausedFlow.begin(),isPausedFlow.end(),flowid);
                isPausedFlow.erase(iter,isPausedFlow.end());
                EV<<"remove flowid is "<< flowid <<", remain flowid = " <<endl;
                for(int i = 0;i < isPausedFlow.size();i++){
                    EV<< isPausedFlow[i] << " ";
                }
                EV<<endl;

                auto pck = new Packet("speedup");
                auto newtag = pck->addTagIfAbsent<HiTag>();
                newtag->setOp(ETHERNET_BFC_SPEEDUP);

                //for bfc speedup cnp
                newtag->setIsSpeedup(true);
                newtag->setLocal_interfaceId(eth_Interface->getInterfaceId());
                newtag->setFlowId(flowid);

                //get数据包的ip_hdr的源地址和目的地址
                offset += bfcHeader->getChunkLength();//前面一步的offset已经加过了macHeader
                auto ip_hdr = packet->peekDataAt<Ipv4Header>(offset);

                L3Address destAddr = ip_hdr->getDestinationAddress();
                L3Address srcAddr = ip_hdr->getSourceAddress();

                newtag->setSrcAddr(srcAddr);
                newtag->setDestAddr(destAddr);

                emit(bfcSpeedupSignal,pck);
                sendcnpspe++;
                EV<<"emit bfcSpeedSignal packet."<<endl;
                delete pck;
           }
           else{
               EV<<"Error, isPaused packet's flow not in isPausedFlow<> vector."<<endl;
           }

           //修改multimap ispaused <eth,upstreamQid> ，删除<eth,upstreamQid>
           bool iserase = false;//记录相同的eth_queue已经被删过一次；
           if(ispaused.find(iface)!= ispaused.end()){// eth in map
               std::pair<iter_ef,iter_ef> itef = ispaused.equal_range(iface);//用指针类型的pair存储所有键值为iface的所有项；
               while(itef.first!=itef.second){
                   if(upstreamQueueID == itef.first->second){//<eth, upstreamQid> in multimap
                       EV<<"Delete eth = "<<iface <<" , upStreamQid = "<<upstreamQueueID<<endl;
                       itef.first = ispaused.erase(itef.first);//del
                       iserase = true;
                       continue;//不要忘了加！！！
                   }
                   itef.first++;
               }
           }else{
                 EV<<"It's error !!!"<< "eth not found in multimap, but packet is isPaused "<<endl;
                }

           if(iserase){//只在第一次被删除时，触发resume，避免重复发送resume.
                //发送resume帧到指定位置。
                auto pck = new Packet("resume");
                auto newtag=pck->addTagIfAbsent<HiTag>();
                newtag->setOp(ETHERNET_BFC_RESUME);
                newtag->setQueueID(upstreamQueueID);
                newtag->setInterfaceId(iface);
                EV<<"Send Resume packet."<<endl;
                emit(bfcResumeSignal,pck);
                delete pck;
               }
           }
        else{
             EV<<"It's error! "<<"pisPaused ->getBfcisPause() = false !"<<endl;
        }//for pisPaused->getBfcisPause()
    }//for packet->findTag<isPause>()

    //更换BFCHeader的upstreamQid = queueid
    auto remove_bfcHeader = packet->removeDataAt<BFCHeader>(offset2);

    int queueid = remove_bfcHeader->getQueueID();
    remove_bfcHeader->setUpstreamQueueID(queueid);
    packet->insertDataAt(remove_bfcHeader,offset2);
    EV<<"pull packt!  Reset packet's upstreamQid = queueid"<<" : "<<queueid<<endl;

    }//for BCN packet

    EV_INFO << "Pulling packet" << EV_FIELD(packet) << EV_ENDL;
    EV<<"queuelength = "<<queue.getBitLength()<<"b"<<endl;
    if(queue.getBitLength()-dataCapacity.get()>=packet->getBitLength()){
        sharedBuffer[switchid][numb]+=b(packet->getBitLength());
    }else if(queue.getBitLength()>=dataCapacity.get()){
        sharedBuffer[switchid][numb]+=b(queue.getBitLength()-dataCapacity.get());
    }

    queue.pop();
    EV<<"after pop queuelength = "<<queue.getBitLength()<<endl;
    auto queueingTime = simTime() - packet->getArrivalTime();
    auto packetEvent = new PacketQueuedEvent();
    packetEvent->setQueuePacketLength(getNumPackets());
    packetEvent->setQueueDataLength(getTotalLength());
    insertPacketEvent(this, packet, PEK_QUEUED, queueingTime, packetEvent);
    increaseTimeTag<QueueingTimeTag>(packet, queueingTime, queueingTime);//记录排队时间

    emit(packetPulledSignal, packet);

    lastResult = doRandomEarlyDetection(packet);
    switch (lastResult) {
    case RANDOMLY_ABOVE_LIMIT:
    case ABOVE_MAX_LIMIT: {
        if (useEcn) {
            //for BFCHeader. Because packet has add BFCHeader after ethernet.
            /*
            auto ethHeader = packet->popAtFront<EthernetMacHeader>();
            auto bfcHeader = packet->popAtFront<BFCHeader>();
            EV<<packet->getFullName()<<" >ECN, packet remove BFCHeader."<<endl;
            */

            IpEcnCode ecn = EcnMarker::getEcn(packet);
            EV<<packet->getFullName()<<" >ECN, packet reset ECN."<<endl;
            EV<<"ecn = "<<ecn<<endl;
            if (ecn != IP_ECN_NOT_ECT) {
                // if next packet should be marked and it is not
                if (markNext && ecn != IP_ECN_CE) {
                    EV<<"set ECN, markNext = false;"<<endl;
                    EcnMarker::setEcn(packet, IP_ECN_CE);
                    markNext = false;
                }
                else {
                    if (ecn == IP_ECN_CE)
                        markNext = true;
                    else{
                        EV<<"set ECN"<<endl;
                        EcnMarker::setEcn(packet, IP_ECN_CE);
                    }

                }
            }

        }
    }
    case RANDOMLY_BELOW_LIMIT:
    case BELOW_MIN_LIMIT:
        break;
    default:
        throw cRuntimeError("Unknown RED result");
    }
    animatePullPacket(packet, outputGate);
    updateDisplayString();
    return packet;
}

BCNqueue::RedResult BCNqueue::doRandomEarlyDetection(const Packet *packet)
{
    int64_t queueLength = queue.getByteLength();
    if (Kmin <= queueLength && queueLength < Kmax) {
        count++;
        const double pb = Pmax * (queueLength - Kmin) / (Kmax - Kmin);
        if (dblrand() < pb) {EV<<"RANDOMLY ABOVE LIMIT"<<endl;
            count = 0;
            return RANDOMLY_ABOVE_LIMIT;
        }
        else{EV<<"RANDOMLY BELOW LIMIT"<<endl;
            return RANDOMLY_BELOW_LIMIT;
        }
    }
    else if (queueLength >= Kmax) {EV<<"ABOVE MAX LIMIT"<<endl;
        count = 0;
        return ABOVE_MAX_LIMIT;
    }
    else {
        count = -1;
    }
    EV<<"BELOW MIN LIMIT"<<endl;
    return BELOW_MIN_LIMIT;
}

void BCNqueue::removePacket(Packet *packet)
{
    Enter_Method("removePacket");
    EV_INFO << "Removing packet" << EV_FIELD(packet) << EV_ENDL;
    queue.remove(packet);
    emit(packetRemovedSignal, packet);
    updateDisplayString();
}

void BCNqueue::removeAllPackets()
{
    Enter_Method("removeAllPackets");
    EV_INFO << "Removing all packets" << EV_ENDL;
    std::vector<Packet *> packets;
    for (int i = 0; i < getNumPackets(); i++)
        packets.push_back(check_and_cast<Packet *>(queue.pop()));
    for (auto packet : packets) {
        emit(packetRemovedSignal, packet);
        delete packet;
    }
    updateDisplayString();
}

bool BCNqueue::canPushSomePacket(cGate *gate) const
{
    if (packetDropperFunction)
        return true;
    if (getMaxNumPackets() != -1 && getNumPackets() >= getMaxNumPackets())
        return false;
    if (getMaxTotalLength() != b(-1) && getTotalLength() >= getMaxTotalLength())
        return false;
    return true;
}

bool BCNqueue::canPushPacket(Packet *packet, cGate *gate) const
{
    if (packetDropperFunction)
        return true;
    if (getMaxNumPackets() != -1 && getNumPackets() >= getMaxNumPackets())
        return false;
    if (getMaxTotalLength() != b(-1) && getMaxTotalLength() - getTotalLength() < packet->getDataLength())
        return false;
    return true;
}

void BCNqueue::handlePacketRemoved(Packet *packet)
{
    Enter_Method("handlePacketRemoved");
    if (queue.contains(packet)) {
        EV_INFO << "Removing packet" << EV_FIELD(packet) << EV_ENDL;
        queue.remove(packet);
        emit(packetRemovedSignal, packet);
        updateDisplayString();
    }
}

bool BCNqueue::BufferManagement(cMessage *msg){

    Packet *packet = check_and_cast<Packet*>(msg);
    int64_t queueLength = queue.getBitLength();
    queuelengthVector.recordWithTimestamp(simTime(), queueLength);
    if(!isOverloaded())
    {
        return true;
    }

    b RemainingBufferSize = sharedBuffer[switchid][numb];
    sharedBufferVector.recordWithTimestamp(simTime(), RemainingBufferSize.get());
    EV << "currentqueuelength = " << queueLength << "b, RemainingBufferSize  = " << RemainingBufferSize.get() << "b, Packet Length is" << packet->getByteLength() <<"B"<<endl;

    maxSize = double(alpha*RemainingBufferSize.get()) ;
    if (queueLength + packet->getBitLength() >  maxSize){
        EV << "it's false" <<endl;
        //std::cout<<"queuelength > "<<maxSize<<endl;
        return false; // drop
    }
    else{
        sharedBuffer[switchid][numb] = RemainingBufferSize - b(packet->getBitLength());
        return true;
    }
}

void BCNqueue::finish(){
    //写入一个基类finish

    recordScalar("send bcn cnp deleration",sendcnpdel);
    recordScalar("send bcn cnp speedup",sendcnpspe);

}

} // namespace inet

