//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "BFCScheduler.h"

namespace inet {

Define_Module(BFCScheduler);

BFCScheduler::~BFCScheduler()
{
    delete[] weights01;
    delete[] buckets01;
}

void BFCScheduler::initialize(int stage)
{
    PacketSchedulerBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {

        weights01 = new unsigned int[providers.size()];
        buckets01 = new unsigned int[providers.size()];

        cStringTokenizer tokenizer(par("weights01"));
        size_t i;
        for (i = 0; i < providers.size() && tokenizer.hasMoreTokens(); ++i)
             buckets01[i] = weights01[i] = utils::atoul(tokenizer.nextToken());

        if (i < providers.size())
             throw cRuntimeError("Too few values given in the weights01 parameter.");
        if (tokenizer.hasMoreTokens())
             throw cRuntimeError("Too many values given in the weights01 parameter.");

        for (auto provider : providers)
            collections.push_back(dynamic_cast<BFCqueue *>(provider));

        numInputs = gateSize("in");

        cModule *radioModule = getParentModule()->getParentModule();EV<<"parentmodule = "<<radioModule<<endl;
        radioModule->subscribe(BFCMac::bfcPausedFrame,this);
        radioModule->subscribe(BFCMac::bfcResumeFrame,this);
        for (int i = 0; i < numInputs; ++i) {
            ispaused[i] = 0;
        }
    }
}

int BFCScheduler::getNumPackets() const
{
    int size = 0;
    for (auto collection : collections)
        if (collection != nullptr)
            size += collection->getNumPackets();
        else
            return -1;
    return size;
}

b BFCScheduler::getTotalLength() const
{
    b totalLength(0);
    for (auto collection : collections)
        if (collection != nullptr)
            totalLength += collection->getTotalLength();
        else
            return b(-1);
    return totalLength;
}

Packet *BFCScheduler::getPacket(int index) const //功能是得到指定index的包，不是指定index的队列
{
    int origIndex = index;

    for (auto collection : collections) {
        auto numPackets = collection->getNumPackets();
        if (index < numPackets)
        {
            return collection->getPacket(index);
        }

        else
            index -= numPackets;
    }
    throw cRuntimeError("Index %i out of range", origIndex);
}

void BFCScheduler::removePacket(Packet *packet)
{
    Enter_Method("removePacket");
    for (auto collection : collections) {
        int numPackets = collection->getNumPackets();
        for (int j = 0; j < numPackets; j++) {
            if (collection->getPacket(j) == packet) {
                collection->removePacket(packet);
                return;
            }
        }
    }
    throw cRuntimeError("Cannot find packet");
}

void BFCScheduler::removeAllPackets()
{
    Enter_Method("removeAllPackets");
    for (auto collection : collections)
        collection->removeAllPackets();
}

bool BFCScheduler::canPullSomePacket(cGate *gate) const
{
    for (int i = 0; i < (int)inputGates.size(); i++) {
        if (ispaused.find(i)->second != 0) // is paused
        {
            continue;
        }else{
            auto inputProvider = providers[i];
            if (inputProvider->canPullSomePacket(inputGates[i]->getPathStartGate()))
                return true;
        }
    }
    return false;
}


int BFCScheduler::schedulePacket()// return Qid index
{
//    功能：决定每次从哪个队列中调度数据包（一次仅在一个队列中，调度一个数据包）
//    1、轮询所有队列
//      ①如果有队列暂停，跳过，询问其他队列；
//      ②

    int firstWeighted = -1;
    int firstNonWeighted = -1;
    uint64_t flowid;

    //选队列
    for (size_t i = 0; i < collections.size(); i++) {

        int inputIndex = getInputGateIndex(i);
        if (inputIndex == inProgressGateIndex || providers[inputIndex]->canPullSomePacket(inputGates[inputIndex]->getPathStartGate()))
        {
            if(buckets01[i] > 0 ){//这里不满足
                buckets01[i]--;
                EV<<"queue i = "<< i <<"; buckets01[i] =  "<<buckets01[i]<<endl;
                return inputIndex;
            }
            else if (firstWeighted == -1 && weights01[i] > 0)
                firstWeighted = (int)i;
            else if (firstNonWeighted == -1 && weights01[i] == 0)
                firstNonWeighted = (int)i;
            }
    }

    if (firstWeighted != -1) {
        for (size_t i = 0; i < providers.size(); ++i)
            buckets01[i] = weights01[i];
         buckets01[firstWeighted]--;
         EV<<"queue ID(firstWeighted) = "<< firstWeighted <<endl;
         return firstWeighted;
     }

    if (firstNonWeighted != -1){
        EV<<"queue ID(firstNonWeighted) = "<< firstNonWeighted <<endl;
        return firstNonWeighted;
    }
    //选队列
    return -1;//怎么会返回 -1呢
}


Packet *BFCScheduler::pullPacket(cGate *gate)//
{
    Enter_Method("pullPacket");
    checkPacketStreaming(nullptr);
    //每包轮询调度
    int index = callSchedulePacket();
    auto packet = providers[index]->pullPacket(inputGates[index]->getPathStartGate());//outputGate;
    take(packet);
    EV_INFO << "Scheduling Queueid " << index <<", packet =" << EV_FIELD(packet) << EV_ENDL;
    handlePacketProcessed(packet);
    emit(packetPulledSignal, packet);
    animatePullPacket(packet, outputGate);
    updateDisplayString();
    return packet;

}


void BFCScheduler::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    if(signalID==BFCMac::bfcPausedFrame||signalID==BFCMac::bfcResumeFrame){
        EV<<"BFCScheduler::receiveSignal(), receive bfc frame"<<endl;
        processbfcframe(obj);
    }
}

void BFCScheduler::processbfcframe(cObject *obj){
    Enter_Method("processbfcframe");
    auto pauseFrame = check_and_cast<const EthernetBfcFrame *>(obj);
    if(pauseFrame->getOpCode()==ETHERNET_BFC_PAUSE){
        ispaused[pauseFrame->getQueueID()]++;

        EV<<"---------Scheduler receive the pause frame to queueID =  "<<pauseFrame->getQueueID()<<" paused"<<endl;

    }else if(pauseFrame->getOpCode()==ETHERNET_BFC_RESUME){
        ispaused[pauseFrame->getQueueID()]--;
        EV<<"----------------Scheduler receive the resume frame  to queueID =  "<<pauseFrame->getQueueID()<<" resumed"<<endl;

    }
}
} // namespace inet

