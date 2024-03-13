/*
 * BFCClassifier.cc
 *
 *  Created on: 2023年10月22日
 *      Author: ergeng2001
 */


//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "BFCClassifier.h"

namespace inet {


Define_Module(BFCClassifier);
int Nums_size;
simsignal_t BFCClassifier::packetClassifiedSignal = cComponent::registerSignal("packetClassified");

void BFCClassifier::initialize(int stage)
{
    PacketClassifierBase::initialize(stage);
    numOutGates = gateSize("out");
    std::vector<int> Nums;
    const char *prios = par("Nums");
    cStringTokenizer tokens(prios);
    while (tokens.hasMoreTokens())
    {
        int temp_nums = atoi(tokens.nextToken());
        Nums.push_back(temp_nums);
    }
        Nums_size = (int)Nums.size();

    if (Nums_size > numOutGates)
        throw cRuntimeError("%d priority values are given, but the module has only %d out gates",
                Nums_size, numOutGates);
    for (int i = 0; i < numOutGates; ++i)
    {
        numbToGateIndexMap[i] = Nums[i];
    }
    numRcvd = 0;
    WATCH(numRcvd);
}

void BFCClassifier::handleMessage(cMessage *msg)
{
    Enter_Method("classifiedPacket");
    Packet *packet = check_and_cast<Packet *>(msg);
    numRcvd++;
    int index = classifyPacket(packet);

    if(!strcmp(packet->getFullName(),"credit"))
    {
        EV<<"send to creditqueue"<<endl;
        send(packet, "creditOut");
    }

    else if (index >= 0){
       // EV<<"send to "<<index<<endl;
        send(packet, "out", index);
    }
    else{
        EV<<"send to default queue"<<endl;
        send(packet, "defaultOut");
    }
    emit(packetClassifiedSignal,packet);

}

int BFCClassifier::classifyPacket(Packet *packet)
{
    if (std::string(packet->getFullName()).find("BFC") != std::string::npos){

        //1、get packet's flowid
        auto& HiPacket = packet -> peekAtFront<ByteCountChunk>();
        int flowID;
        for(auto& region:packet->peekData()->getAllTags<HiTag>()){
            flowID = region.getTag()->getFlowId();
        }

        //2、check map<flowid,queueid>
        int queueID;
        if(flowid_queue.find(flowID)!=flowid_queue.end()){//2.1 flowid in <flowid,queueid>; return queue;
            queueID = flowid_queue.find(flowID)->second;
            b offset(0);
            //2.1.1 insert packet bfcHeader
            auto ethHeader = packet->peekDataAt<EthernetMacHeader>(offset);
            offset += ethHeader->getChunkLength();
            auto bfcHeader = packet->removeDataAt<BFCHeader>(offset);
            bfcHeader ->setQueueID(queueID);
            EV<<packet<<"flowid in <flowid,queueid>, bfcHeader set queueid = "<<bfcHeader ->getQueueID()<<endl;
            packet->insertDataAt(bfcHeader, offset);

            //2.1.2 before return queueid, if packet is islast packet, delete <flowid, queueid>
            bool last = false;
            for(auto& region: packet->peekData()->getAllTags<HiTag>()){
                 last = region.getTag()->isLastPck();
            }
            if(last){//delete <flowid, queueid>
                fq_Iter = flowid_queue.find(flowID);
                flowid_queue.erase(fq_Iter);
            }

            //2.1.3 for return queueid
            for(auto it=numbToGateIndexMap.begin(); it != numbToGateIndexMap.end(); it++) {
                  if(it->second == queueID){
                      EV<<"Scheduling the packet to queue Numb  = "<< it->first <<endl;
                      return it->first;
                  }
             }

        }else{//2.2 flowid not in <flowid,queueid>,insert map<flowid,queueid>
                //2.2.1 check empty queueid
                int forset_queueid; //save for set queueid
                cModule *radioModule_HiQueue = getParentModule();//Classifier -> BFCQueue
                bool notallfull = false;
                for(auto& c: radioModule_HiQueue->getSubmoduleNames()){
                    if(c.find("Num")!= std::string::npos){//find BFCqueue
                        BFCqueue *bfcQueue = check_and_cast<BFCqueue*>(radioModule_HiQueue->getSubmodule(c.c_str()));
                        if(bfcQueue->queue.getBitLength()==0){//return queuelength == 0
                            int c_size =c.size();
                            int np = c.find_first_of('_');
                            std::string c_end = c.substr(np+1, c_size - 1);
                            EV<<"BFCQueue == 0, "<<c<<" queue.getBitLength()= "<< bfcQueue->queue.getBitLength()<<endl;
                            forset_queueid = std::stoi(c_end);
                            EV<<"forset_queueid  = "<<forset_queueid <<endl;
                            notallfull = true;//
                            break;
                        }else{//BFCqueue length not 0!
                            EV<<"BFCqueue length not 0! "<<c<<" queue.getBitLength()= "<< bfcQueue->queue.getBitLength()<<endl;
                            continue;
                        }
                    }//for BFCqueue
                }// for BFCqueue all modules

                //2.2.2 all queues are not empty，srand queueid
                if(!notallfull){
                    EV<<"Now all queues are not empty , srand flow to any queue."<<endl;
                    srand( (unsigned)time( NULL ) );
                    forset_queueid = rand()%(Nums_size-0)+0; //对于大于一条队列
                    EV<<"num.size = "<< Nums_size <<endl;
                    EV<<"For set queue = "<< forset_queueid <<endl;
                }

                //2.2.3 insert <flowid,queueid>
                flowid_queue.insert(std::pair<int,int>(flowID,forset_queueid));

                //2.2.4 set bfcHeader queueid
                b offset(0);
                auto ethHeader = packet->peekDataAt<EthernetMacHeader>(offset);
                offset += ethHeader->getChunkLength();
                auto bfcHeader = packet->removeDataAt<BFCHeader>(offset);
                bfcHeader ->setQueueID(forset_queueid);
                EV<<packet<<"  bfcHeader ->getQueueID(forset_queueid)  = "<<bfcHeader ->getQueueID()<<endl;
                packet->insertDataAt(bfcHeader, offset);

                //2.2.5 return for set queueid
                for(auto it=numbToGateIndexMap.begin(); it != numbToGateIndexMap.end(); it++) {
                      if(it->second == forset_queueid){
                          EV<<"Scheduling the packet to queue Numb  = "<< it->first <<endl;
                          return it->first;
                      }
                 }
            }//for flowid not in <flowid,queueid>
        }//for BFC packet
    if (std::string(packet->getFullName()).find("arp") != std::string::npos)
        return -1;

    return -1;//for error!
}

void BFCClassifier::refreshDisplay() const
{
    char buf[33] = "";
    if (numRcvd > 0)
        sprintf(buf + strlen(buf), "classified:%d ", numRcvd);
    getDisplayString().setTagArg("t", 0, buf);


}
}// namespace inet



