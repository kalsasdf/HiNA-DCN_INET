/*
 * BCNClassifier.cc
 *
 *  Created on: 2023年10月23日
 *      Author: ergeng2001
 */


//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "BCNClassifier.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>

#include "../BCNqueue/BCNqueue.h"

#include "inet/HiNA/Ethernet/HiQueue/REDPFCQueue/REDPFCQueue.h"
#include "inet/HiNA/Ethernet/HiQueue/ABMQueue/ABMQueue.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/Simsignals.h"
#include "inet/linklayer/common/EtherType_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/linklayer/ethernet/common/EthernetControlFrame_m.h"
#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "inet/networklayer/common/NetworkInterface.h"
#include "inet/physicallayer/wired/ethernet/EthernetSignal_m.h"


#include "inet/linklayer/ethernet/base/EthernetMacBase.h"
#include "inet/HiNA/Messages/HiTag/HiTag_m.h"
#include "inet/HiNA/Messages/PfcFrame/EthernetPfcFrame_m.h"
#include "inet/HiNA/Messages/BFCHeader/BFCHeader_m.h"

#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/transportlayer/udp/UdpHeader_m.h"
#include "inet/HiNA/Messages/HPCC/INTHeader_m.h"
#include "inet/HiNA/Ethernet/HiQueue/HiScheduler/WrrScheduler.h"

#include "inet/common/packet/Packet.h"

namespace inet {


Define_Module(BCNClassifier);
int Numsbcn_size;
simsignal_t BCNClassifier::packetClassifiedSignal = cComponent::registerSignal("packetClassified");

void BCNClassifier::initialize(int stage)
{
    PacketClassifierBase::initialize(stage);
    numOutGates = gateSize("out");
    std::vector<int> priorities;
    std::vector<int> Nums;
    const char *prios = par("Nums");
    cStringTokenizer tokens(prios);
    while (tokens.hasMoreTokens())
    {
        int temp_nums = atoi(tokens.nextToken());
        Nums.push_back(temp_nums);
    }
        Numsbcn_size = (int)Nums.size();

    if (Numsbcn_size > numOutGates)
        throw cRuntimeError("%d priority values are given, but the module has only %d out gates",
                Numsbcn_size, numOutGates);
    for (int i = 0; i < numOutGates; ++i)
    {
        numbToGateIndexMap[i] = Nums[i];
    //    EV<<"numbToGateIndexMap["<<i<<"] = "<<Nums[i]<<endl;
//        EV_INFO<<"priorityToGate["<<i<<"] = "<<priorities[i]<<endl;
    }

    numRcvd = 0;
    WATCH(numRcvd);

}

void BCNClassifier::handleMessage(cMessage *msg)
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
    else if(!strcmp(packet->getFullName(),"routerCNP")){
        send(packet, "defaultOut");
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

int BCNClassifier::classifyPacket(Packet *packet)
{


    if ((std::string(packet->getFullName()).find("BCN") != std::string::npos)||(std::string(packet->getFullName()).find("BFC") != std::string::npos)){
   //为相同flowid的数据包插上相同的forset_queueid
//   EV<<"packet= "<<packet<<endl;
   //获取HiTag中携带的flowID。作为flowid_queue map索引的key值；
   auto& HiPacket = packet -> peekAtFront<ByteCountChunk>();
   int flowID;
   for(auto& region:packet->peekData()->getAllTags<HiTag>()){
       flowID = region.getTag()->getFlowId();
   }

   //记录数据包的flowid_forset_queueid [map类型]
   int queueID;
   if(flowid_queue.find(flowID)!=flowid_queue.end()){//flowid in flowid_queue map; return queue;
       queueID = flowid_queue.find(flowID)->second;
       b offset(0);
       auto ethHeader = packet->peekDataAt<EthernetMacHeader>(offset);
       offset += ethHeader->getChunkLength();
       auto bfcHeader = packet->removeDataAt<BFCHeader>(offset);
       bfcHeader ->setQueueID(queueID);
       EV<<packet<<"flowid in flowid_queue map; bfcHeader ->getQueueID(forset_queueid); = "<<bfcHeader ->getQueueID()<<endl;
       packet->insertDataAt(bfcHeader, offset);
       for (auto it=numbToGateIndexMap.begin(); it != numbToGateIndexMap.end(); it++) {
               if(it->second == queueID){
                   EV<<"Scheduling the packet to queue Numb  = "<< it->first <<endl;
                   return it->first;
               }
           }

   }else{//flowid not in flowid_queue map; insert flowid
       int forset_queueid;
          cModule *radioModule_HiQueue = getParentModule();//当前HiQueue


/*
// for BCN 返回最短的队列长度；
          if (std::string(packet->getFullName()).find("BCN") != std::string::npos){
              EV<<"packet = "<<packet <<endl;
          if(flowID!=lastflowid){
              EV<<"this flowid = "<<flowID<<endl;
              EV<<"lastflowid= "<<lastflowid<<endl;
          for(auto& c: radioModule_HiQueue->getSubmoduleNames()){
              EV<<"in c "<<endl;

              if(c.find("Num")!= std::string::npos){//是BCNqueue队列
                  EV<<"in num"<<endl;

                  BCNqueue *bfcQueue = check_and_cast<BCNqueue*>(radioModule_HiQueue->getSubmodule(c.c_str()));

                  EV<<"minlength = "<<minlength<<endl;
                  if(minlength > bfcQueue->queue.getBitLength()){
                      EV<<"minlength = "<<minlength<<endl;

                  int c_size =c.size();
                  int np = c.find_first_of('_');
                  std::string c_end = c.substr(np+1, c_size - 1);
                  EV<<"BCNqueue :  "<<c<<" queue.getBitLength()= "<< bfcQueue->queue.getBitLength()<<endl;
                  tempqueueid = std::stoi(c_end);

                  if(tempqueueid == lastqueueid)
                  {
                      EV<<"tempqueueid = "<<tempqueueid <<endl;
                      EV<<"lastqueueid = "<<lastqueueid <<endl;
                      continue;
                  }else{
                      EV<<"tempqueueid = "<<tempqueueid <<endl;
                      EV<<"lastqueueid = "<<lastqueueid <<endl;
                      minlength = bfcQueue->queue.getBitLength();
                      minqueueid = tempqueueid;

                  }

                  }
                  continue;
              }
          }

          forset_queueid = minqueueid;
          minlength = std::numeric_limits<int64_t>::max();
          minqueueid = -1;
          lastqueueid = forset_queueid;
          EV<<"lastqueueid = "<<lastqueueid <<endl;
          lastflowid = flowID;
          EV<<"BCN: For set queue = "<< forset_queueid <<endl;
          }
// for BCN 返回最短的队列长度；

 */

//          }else if(std::string(packet->getFullName()).find("BFC") != std::string::npos){

// for BFC 队列都不为空后，返回随机队列
          bool notallfull = false;
          for(auto& c: radioModule_HiQueue->getSubmoduleNames()){

              if(c.find("Num")!= std::string::npos){//是BCNqueue队列

                  BCNqueue *bfcQueue = check_and_cast<BCNqueue*>(radioModule_HiQueue->getSubmodule(c.c_str()));

                  if(bfcQueue->queue.getBitLength()==0){//返回该数据包被分配到的队列长度为0的队列

                      int c_size =c.size();
                      int np = c.find_first_of('_');
                      std::string c_end = c.substr(np+1, c_size - 1);
                      EV<<"BCNqueue == 0, "<<c<<" queue.getBitLength()= "<< bfcQueue->queue.getBitLength()<<endl;
                      forset_queueid = std::stoi(c_end);
                      EV<<"forset_queueid  = "<<forset_queueid <<endl;
                      notallfull = true;
                      break;
                  }else{
                      EV<<"Now BFCHeader, "<<c<<" queue.getBitLength()= "<< bfcQueue->queue.getBitLength()<<endl;
                      continue;
                  }//返回第一个队列长度为0的队列

              }//for BCNqueue

         }// for HiQueue的所有队列

          //如果队列全部都占满，那么就随机分配流到随机队列
          if(!notallfull){

              EV<<"Now all queue is full, srand flow to any queue."<<endl;
              srand( (unsigned)time( NULL ) );
              forset_queueid = rand()%(Numsbcn_size-0)+0; //对于大于一条队列
//              forset_queueid = 0; //对于只有一个队列的清楚，num = 0;

              EV<<"num.size = "<< Numsbcn_size <<endl;
              EV<<"For set queue = "<< forset_queueid <<endl;

          }
//          }
// for BFC 队列都不为空后，返回随机队列

           flowid_queue.insert(std::pair<int,int>(flowID,forset_queueid));
           b offset(0);
           auto ethHeader = packet->peekDataAt<EthernetMacHeader>(offset);
           offset += ethHeader->getChunkLength();
           auto bfcHeader = packet->removeDataAt<BFCHeader>(offset);
           bfcHeader ->setQueueID(forset_queueid);
           EV<<packet<<"  bfcHeader ->getQueueID(forset_queueid)  = "<<bfcHeader ->getQueueID()<<endl;
           packet->insertDataAt(bfcHeader, offset);
           for (auto it=numbToGateIndexMap.begin(); it != numbToGateIndexMap.end(); it++) {
                   if(it->second == forset_queueid){
                       EV<<"Scheduling the packet to queue Numb  = "<< it->first <<endl;
                       return it->first;
                   }
               }

   }

}//for "BCN" packet.

    if (std::string(packet->getFullName()).find("arp") != std::string::npos)
        return -1;

    return -1;
}

void BCNClassifier::refreshDisplay() const
{
    char buf[33] = "";
    if (numRcvd > 0)
        sprintf(buf + strlen(buf), "classified:%d ", numRcvd);
    getDisplayString().setTagArg("t", 0, buf);


}
}// namespace inet



