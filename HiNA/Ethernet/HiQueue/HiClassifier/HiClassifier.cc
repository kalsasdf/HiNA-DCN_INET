//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "HiClassifier.h"

namespace inet {


Define_Module(HiClassifier);

void HiClassifier::initialize(int stage)
{
    PacketClassifierBase::initialize(stage);
    numOutGates = gateSize("out");
    std::vector<int> priorities;
    const char *prios = par("priorities");
    cStringTokenizer tokens(prios);
    while (tokens.hasMoreTokens())
    {
        int temp_priority = atoi(tokens.nextToken());
        priorities.push_back(temp_priority);
    }
    int numPriorities = (int)priorities.size();
    if (numPriorities > numOutGates)
        throw cRuntimeError("%d priority values are given, but the module has only %d out gates",
                numPriorities, numOutGates);
    for (int i = 0; i < numOutGates; ++i)
    {
        priorityToGateIndexMap[i] = priorities[i];
//        EV_INFO<<"priorityToGate["<<i<<"] = "<<priorities[i]<<endl;
    }

    numRcvd = 0;
    WATCH(numRcvd);
}

void HiClassifier::handleMessage(cMessage *msg)
{
    Packet *packet = check_and_cast<Packet *>(msg);
    numRcvd++;
    int index = classifyPacket(packet);
    if(!strcmp(packet->getFullName(),"credit"))
    {
        EV<<"send to creditqueue"<<endl;
        send(packet, "creditOut");
    }
    else if (index >= 0){
        EV<<"send to "<<index<<endl;
        send(packet, "out", index);
    }
    else{
        EV<<"send to default queue"<<endl;
        send(packet, "defaultOut");
    }

}

int HiClassifier::classifyPacket(Packet *packet)
{
    int priority;
    int flowid;
    for (auto& region : packet->peekData()->getAllTags<HiTag>()){
        priority = region.getTag()->getPriority();
        flowid = region.getTag()->getFlowId();
    }
    if (std::string(packet->getFullName()).find("arp") != std::string::npos)
        return -1;
    EV<<"Scheduling priority of the received packet = "<<priority<<", Flow id = "<< flowid<<endl;
    for (auto it=priorityToGateIndexMap.begin(); it != priorityToGateIndexMap.end(); it++) {
        if(it->second==priority)
            return it->first;
    }
    return -1;
}

void HiClassifier::refreshDisplay() const
{
    char buf[20] = "";
    if (numRcvd > 0)
        sprintf(buf + strlen(buf), "rcvd:%d ", numRcvd);
    getDisplayString().setTagArg("t", 0, buf);
    updateDisplayString();
}
} // namespace inet

