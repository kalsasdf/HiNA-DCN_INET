//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "HiPriorityScheduler.h"

namespace inet {

Define_Module(HiPriorityScheduler);

void HiPriorityScheduler::initialize(int stage)
{
    PacketSchedulerBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        for (auto provider : providers)
            collections.push_back(dynamic_cast<IPacketCollection *>(provider));
        numInputs = gateSize("in");

        cModule *radioModule = getParentModule()->getParentModule();EV<<"parentmodule = "<<radioModule<<endl;
        radioModule->subscribe(HiEthernetMac::pfcPausedFrame,this);
        radioModule->subscribe(HiEthernetMac::pfcResumeFrame,this);
        for (int i = 0; i < numInputs; ++i) {
            ispaused[i] = false;
        }
    }
}

int HiPriorityScheduler::getNumPackets() const
{
    int size = 0;
    for (auto collection : collections)
        if (collection != nullptr)
            size += collection->getNumPackets();
        else
            return -1;
    return size;
}

b HiPriorityScheduler::getTotalLength() const
{
    b totalLength(0);
    for (auto collection : collections)
        if (collection != nullptr)
            totalLength += collection->getTotalLength();
        else
            return b(-1);
    return totalLength;
}

Packet *HiPriorityScheduler::getPacket(int index) const
{
    int origIndex = index;
    for (auto collection : collections) {
        auto numPackets = collection->getNumPackets();
        if (index < numPackets)
            return collection->getPacket(index);
        else
            index -= numPackets;
    }
    throw cRuntimeError("Index %i out of range", origIndex);
}

void HiPriorityScheduler::removePacket(Packet *packet)
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

void HiPriorityScheduler::removeAllPackets()
{
    Enter_Method("removeAllPackets");
    for (auto collection : collections)
        collection->removeAllPackets();
}

bool HiPriorityScheduler::canPullSomePacket(cGate *gate) const
{
    for (int i = 0; i < (int)inputGates.size(); i++) {
        if (ispaused.find(i)->second) // is paused
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

int HiPriorityScheduler::schedulePacket()
{
    for (size_t i = 0; i < collections.size(); i++) {
        if (ispaused.find(i)->second) // is paused
        {
            continue;
        }
        else{
            int inputIndex = getInputGateIndex(i);
            if (inputIndex == inProgressGateIndex || providers[inputIndex]->canPullSomePacket(inputGates[inputIndex]->getPathStartGate()))
                return inputIndex;
        }
    }
    return -1;
}

void HiPriorityScheduler::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    if(signalID==HiEthernetMac::pfcPausedFrame||signalID==HiEthernetMac::pfcResumeFrame){
        EV<<"HiPriorityScheduler::receiveSignal(), receive pfc frame"<<endl;
        processpfcframe(obj);
    }
}

void HiPriorityScheduler::processpfcframe(cObject *obj){
    Enter_Method("processpfcframe");
    auto pauseFrame = check_and_cast<const EthernetPfcFrame *>(obj);
    if(pauseFrame->getOpCode()==ETHERNET_PFC_PAUSE){
        ispaused[pauseFrame->getPriority()]=true;EV<<"priority "<<pauseFrame->getPriority()<<" paused"<<endl;
    }else if(pauseFrame->getOpCode()==ETHERNET_PFC_RESUME){
        ispaused[pauseFrame->getPriority()]=false;EV<<"priority "<<pauseFrame->getPriority()<<" resumed"<<endl;
    }
}
} // namespace inet

