//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "BFCqueue.h"

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

Define_Module(BFCqueue);

simsignal_t BFCqueue::bfcPausedSignal =
        cComponent::registerSignal("bfcPaused");
simsignal_t BFCqueue::bfcResumeSignal =
        cComponent::registerSignal("bfcResume");

b BFCqueue::sharedBuffer[100][100]={};

void BFCqueue::initialize(int stage)
{
    PacketQueueBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        queue.setName("storage");
        producer = findConnectedModule<IActivePacketSource>(inputGate);
        collector = findConnectedModule<IActivePacketSink>(outputGate);
        packetCapacity = par("packetCapacity");
        EV<<"packetCapacity = "<<packetCapacity  <<endl;//4000个包
        dataCapacity = b(par("dataCapacity"));
        EV<<"dataCapacity = "<<dataCapacity <<endl;//50MB
        switchid=findContainingNode(this)->getId();EV<<"switchid = "<<switchid<<endl;
        numb = par("numb");
        sharedBuffer[switchid][numb] = b(par("sharedBuffer"));
        EV<<"sharedBuffer[switchid][numb] = "<<sharedBuffer[switchid][numb] <<endl;//1.5MB
        headroom = b(par("headroom"));//179200b == 22400B
        EV<<"headroom ="<<headroom <<endl;
        useBfc = par("useBfc");
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

IPacketDropperFunction *BFCqueue::createDropperFunction(const char *dropperClass) const
{
    if (strlen(dropperClass) == 0)
        return nullptr;
    else
        return check_and_cast<IPacketDropperFunction *>(createOne(dropperClass));
}

IPacketComparatorFunction *BFCqueue::createComparatorFunction(const char *comparatorClass) const
{
    if (strlen(comparatorClass) == 0)
        return nullptr;
    else
        return check_and_cast<IPacketComparatorFunction *>(createOne(comparatorClass));
}

bool BFCqueue::isOverloaded() const
{
    return (packetCapacity != -1 && getNumPackets() > packetCapacity) ||
           (dataCapacity != b(-1) && getTotalLength() > dataCapacity);
}

int BFCqueue::getNumPackets() const
{
    return queue.getLength();
}

Packet *BFCqueue::getPacket(int index) const
{
    if (index < 0 || index >= queue.getLength())
        throw cRuntimeError("index %i out of range", index);
    return check_and_cast<Packet *>(queue.get(index));
}

void BFCqueue::handleMessage(cMessage *message)
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

void BFCqueue::pushPacket(Packet *packet, cGate *gate)
{

    Enter_Method("pushPacket");
    take(packet);
    cNamedObject packetPushStartedDetails("atomicOperationStarted");
    emit(packetPushStartedSignal, packet, &packetPushStartedDetails);
    EV_INFO << "Pushing packet" << EV_FIELD(packet) << EV_ENDL;
    queue.insert(packet);

    int iface = packet->addTagIfAbsent<InterfaceInd>()->getInterfaceId(); //iface is the packet form port;not the now port;
    if (std::string(packet->getFullName()).find("BFC") != std::string::npos){//不是arp
    if(useBfc){
        if(queue.getByteLength()>=XOFF||(queue.getBitLength()>dataCapacity.get()&&sharedBuffer[switchid][numb]<headroom)){

            EV<<"###########When push packet, queueleng is overload! ############"<<endl;
            cModule *radioModule = getParentModule()->getParentModule(); //BFCqueue -> BFCQueue -> BFCEthernetInterface
            NetworkInterface* eth_Interface = check_and_cast<NetworkInterface*>(radioModule); //queue -> ethid

            EV<<"Packet push in eth Interface = "<<radioModule<<endl; //radioModule == BFCEthernetInterface
            EV<<"Packet push in switch = "<<radioModule->getParentModule()<<endl;

            if(queue.getByteLength()>=XOFF){
                EV<<"queue.getByteLength()>=XOFF: "<<queue.getByteLength() << " >= "<<XOFF<<endl;
            }else{
                EV<<"queue.getBitLength()>dataCapacity.get()&&sharedBuffer[switchid][numb]<headroom"<<endl;
                EV<<"queue.getBitLength()>dataCapacity.get(): "<<queue.getBitLength()<<" > "<<dataCapacity.get()<<endl;
                EV<<"sharedBuffer[switchid][numb]<headroom: "<< sharedBuffer[switchid][numb] << " < "<< headroom<<endl;
            }

            //1、get packet's BFCHeader upstreamQid
            b offset(0);
            auto ethHeader = packet->peekDataAt<EthernetMacHeader>(offset);
            offset += ethHeader->getChunkLength();
            auto bfcHeader = packet->peekDataAt<BFCHeader>(offset);
            int upstreamQueueID = bfcHeader->getUpstreamQueueID();

            //2、get packet's flowid，for first isPaused packet
            uint32_t flowid;
            for (auto& region : packet->peekData()->getAllTags<HiTag>()){
                flowid = region.getTag()->getFlowId();
            }

            //3、for check isPausedFlow vector
            EV<<"In isPausedFlow vector, the flows are: "<<endl;
            for(int i = 0;i < isPausedFlow.size();i++){
                EV<< isPausedFlow[i] << " ";
            }
            EV<<endl;

            //4、check addTag<isPause> or not. Just for the first isPaused packet.
            if(std::find(isPausedFlow.begin(),isPausedFlow.end(),flowid) !=isPausedFlow.end() ){//flowid in vector
                //已经存在，即不再发送paused
            }else{
                //flowid not in vector, addTag<isPause>
                auto PauseTag = packet -> addTag<isPause>();
                PauseTag->setBfcisPause(true);
                isPausedFlow.push_back(flowid);
                EV<<"Insert flowid is "<<flowid <<endl;
                EV<<"Now in isPausedFlow vector, the flows are: "<<endl;
                for(int i = 0;i < isPausedFlow.size();i++){
                    EV<< isPausedFlow[i] << " ";
                }
                EV<<endl;
            }

            //5、check <eth,upstreamQid> in Multimap or not.
            if(ispaused.find(iface)!= ispaused.end()){// eth in map
                std::pair<iter_ef,iter_ef> itef = ispaused.equal_range(iface);//用指针类型的pair存储所有键值为iface的所有项；
                bool isin = false;//用于记录eth_upstreamQ是否在ispaused里面；
                while(itef.first!=itef.second){
                    if(upstreamQueueID == itef.first->second){
                        isin = true;//<eth,upstreamQid> in map.
                    }
                    itef.first++;
                }
                if(!isin){//<eth,upstreamQid> not in map；

                    //① insert <eth,upstreamQid>;
                    ispaused.insert(std::pair<int,int>(iface,upstreamQueueID));
                    //② send bfcPausedSignal
                    auto pck = new Packet("pause");
                    auto newtag=pck->addTagIfAbsent<HiTag>();
                    newtag->setOp(ETHERNET_BFC_PAUSE);
                    newtag->setQueueID(upstreamQueueID);
                    newtag->setInterfaceId(iface);
                    emit(bfcPausedSignal,pck);
                    delete pck;
                    EV<<"Send bfcPausedSignal, eth = "<<iface<<", upstreamQid = "<<upstreamQueueID<<endl;
                }

            }else{//eth not in map
                if(ispaused.empty()){// Multimap is empty
                    //① insert <eth,upstreamQid>
                    ispaused.insert(std::pair<int,int>(iface,upstreamQueueID));
                    iter_ef efptr;
                    efptr = ispaused.begin();
                }else{//Multimap is not empty
                    ispaused.insert(std::pair<int,int>(iface,upstreamQueueID));
                }
                    //② send bfcPausedSignal
                    auto pck = new Packet("pause");
                    auto newtag=pck->addTagIfAbsent<HiTag>();
                    newtag->setOp(ETHERNET_BFC_PAUSE);
                    newtag->setQueueID(upstreamQueueID);
                    newtag->setInterfaceId(iface);
                    emit(bfcPausedSignal,pck);
                    delete pck;
                    EV<<"Send bfcPausedSignal, eth = "<<iface<<", upstreamQ = "<<upstreamQueueID<<endl;
                }
        }//for queuelength is overload!
    }//for useBFC
  }//for BFC packet
    if (collector != nullptr && getNumPackets() != 0)
        collector->handleCanPullPacketChanged(outputGate->getPathEndGate());
    cNamedObject packetPushEndedDetails("atomicOperationEnded");
    emit(packetPushEndedSignal, nullptr, &packetPushEndedDetails);
    updateDisplayString();
}


Packet *BFCqueue::pullPacket(cGate *gate)
{
    Enter_Method("pullPacket");
    auto packet = check_and_cast<Packet *>(queue.front());
    int iface = packet->addTagIfAbsent<InterfaceInd>()->getInterfaceId();

    if (std::string(packet->getFullName()).find("BFC") != std::string::npos){
        //1、get packet BFCHeader upstreamQid；
        b offset(0);
        b offset2(0);
        auto ethHeader = packet->peekDataAt<EthernetMacHeader>(offset);
        offset += ethHeader->getChunkLength();// for get BFCHeader
        offset2 +=ethHeader->getChunkLength();//记录mac头部的chunk大小
        auto bfcHeader = packet->peekDataAt<BFCHeader>(offset);
        int upstreamQueueID = bfcHeader->getUpstreamQueueID();

        //2、get flowid
        uint32_t flowid;
        for (auto& region : packet->peekData()->getAllTags<HiTag>()){
            flowid = region.getTag()->getFlowId();
        }

        //3、check isPause packet
        auto pisPaused = packet ->findTag<isPause>();
        if(pisPaused != nullptr){
        if(pisPaused->getBfcisPause()){
            EV<<"###########Now ispaused packet is pulled, Send resume to <eth,upstreamQid> #############"<<endl;

            cModule *radioModule = getParentModule()->getParentModule();//BFCqueue -> BFCQueue -> eth
            NetworkInterface* eth_Interface = check_and_cast<NetworkInterface*>(radioModule);
            EV<<"Pull packet from eth = "<<radioModule<<endl;
            EV<<"Pull packet from switch = "<<radioModule->getParentModule()<<endl;

            EV<<"queue.getByteLength(): XON = "<<queue.getByteLength() << " : "<<XON<<endl;
            EV<<"queue.getBitLength(): dataCapacity.get(): "<<queue.getBitLength()<<" : "<<dataCapacity.get()<<endl;
            EV<<"sharedBuffer[switchid][numb]: headroom: "<< sharedBuffer[switchid][numb] << " : "<< headroom<<endl;

            EV<<"Now pull, isPausedFlow vector: "<<endl;
            for(int i = 0;i < isPausedFlow.size();i++){
                    EV<<isPausedFlow[i] << " ";
            }
            EV<<endl;

            //4、check isPausedFlow, send bfcResumeSignal
            if(std::find(isPausedFlow.begin(),isPausedFlow.end(),flowid) !=isPausedFlow.end() ){//flow in vector

                //① remove isPaused flowid
                auto iter = std::remove(isPausedFlow.begin(),isPausedFlow.end(),flowid);
                isPausedFlow.erase(iter,isPausedFlow.end());
                EV<<"remove flowid is "<< flowid <<", remain flowid = " <<endl;
                for(int i = 0;i < isPausedFlow.size();i++){
                        EV<< isPausedFlow[i] << " ";
                }
                EV<<endl;

                //② remove <eth,upstreamQid>
                bool iserase = false;
                if(ispaused.find(iface)!= ispaused.end()){// eth in map
                    std::pair<iter_ef,iter_ef> itef = ispaused.equal_range(iface);//用指针类型的pair存储所有键值为iface的所有项；
                    while(itef.first!=itef.second){
                         if(upstreamQueueID == itef.first->second){//<eth,upstreamQid> in Multimap
                             EV<<"Delete eth = "<<iface <<" , upStreamQueue = "<<upstreamQueueID<<endl;
                             itef.first = ispaused.erase(itef.first);//删除该<eth,queue>键值对
                             iserase = true;
                             continue;//不要忘了加！！！
                         }
                         itef.first++;
                     }

                }else{
                    EV<<"It's error !!!"<< "packet is isPaused, but iface(eth) not found in Multimap<eth,upstreamQid>, iface = "<<iface<<endl;
                }

                if(iserase){
                    auto pck = new Packet("resume");
                    auto newtag=pck->addTagIfAbsent<HiTag>();
                    newtag->setOp(ETHERNET_BFC_RESUME);
                    newtag->setQueueID(upstreamQueueID);
                    newtag->setInterfaceId(iface);
                    emit(bfcResumeSignal,pck);
                    delete pck;
                }else{
                    EV<<"It's error! "<<"packet is isPaused, but <eth,upstreamQid> not in Multimap!"<<endl;
                }
            }else{
                EV<<"It's error! "<<"packet is isPaused, but flowid not in vector!"<<endl;
            }
        }//for isPaused packet

        //5、Change BFCHeader upstreamQid = queueid
        auto remove_bfcHeader = packet->removeDataAt<BFCHeader>(offset2);

        int queueid = remove_bfcHeader->getQueueID();
        remove_bfcHeader->setUpstreamQueueID(queueid);
        packet->insertDataAt(remove_bfcHeader,offset2);
        EV<<"pull packt!  Reset packet's upstreamQid = queueid"<<" : "<<queueid<<endl;

        }//for isPaused packet
    }//for BFC packet

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

BFCqueue::RedResult BFCqueue::doRandomEarlyDetection(const Packet *packet)
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

void BFCqueue::removePacket(Packet *packet)
{
    Enter_Method("removePacket");
    EV_INFO << "Removing packet" << EV_FIELD(packet) << EV_ENDL;
    queue.remove(packet);
    emit(packetRemovedSignal, packet);
    updateDisplayString();
}

void BFCqueue::removeAllPackets()
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

bool BFCqueue::canPushSomePacket(cGate *gate) const
{
    if (packetDropperFunction)
        return true;
    if (getMaxNumPackets() != -1 && getNumPackets() >= getMaxNumPackets())
        return false;
    if (getMaxTotalLength() != b(-1) && getTotalLength() >= getMaxTotalLength())
        return false;
    return true;
}

bool BFCqueue::canPushPacket(Packet *packet, cGate *gate) const
{
    if (packetDropperFunction)
        return true;
    if (getMaxNumPackets() != -1 && getNumPackets() >= getMaxNumPackets())
        return false;
    if (getMaxTotalLength() != b(-1) && getMaxTotalLength() - getTotalLength() < packet->getDataLength())
        return false;
    return true;
}

void BFCqueue::handlePacketRemoved(Packet *packet)
{
    Enter_Method("handlePacketRemoved");
    if (queue.contains(packet)) {
        EV_INFO << "Removing packet" << EV_FIELD(packet) << EV_ENDL;
        queue.remove(packet);
        emit(packetRemovedSignal, packet);
        updateDisplayString();
    }
}

bool BFCqueue::BufferManagement(cMessage *msg){

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


} // namespace inet

