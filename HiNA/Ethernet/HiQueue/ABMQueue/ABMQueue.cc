//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "ABMQueue.h"

namespace inet {

Define_Module(ABMQueue);

uint32_t ABMQueue::congestedNum[100][11]={};

bool ABMQueue::BufferManagement(cMessage *msg){

    Packet *packet = check_and_cast<Packet*>(msg);
    int64_t queueLength = queue.getBitLength();
    HiEthernetMac *radioModule = check_and_cast<HiEthernetMac*>(getParentModule() -> getParentModule() -> getSubmodule("mac"));
    DeqRate = radioModule->deqrate[priority];
    if(!isOverloaded())
    {
        return true;
    }
    // ��һ��RTT��alphaֵȡ1024��ȷ������Incast
    // Ŀǰ����������alpha�����ݰ����ȼ�Ϊ0��alphaΪ16/63��CNP��Pause�������ȼ�Ϊ99��alphaΪ32/63
//    if(packet->getifFirstRTT())
//        alpha = 1024;
//    else if(priority <= 11){
//        alpha = alphas[priority];
//    }
    EV << "alpha = " << alpha << endl;
    b RemainingBufferSize = sharedBuffer[switchid];  // ��ǰ������������ʣ���С
    EV << "currentqueuelength is " << queueLength << "b, RemainingBufferSize " << RemainingBufferSize.get() << "b, Packet Length is" << packet->getByteLength() <<"B"<<endl;
    uint32_t numberofP = congestedNum[switchid][priority] ? congestedNum[switchid][priority] : 1 ;
    maxSize = double(alpha*(RemainingBufferSize.get())/numberofP) * DeqRate ;
    EV << "Threshold is " << maxSize << "b, congestedNum is " << congestedNum[switchid][priority] << ", congestion state is " << congested << ", DeqRate = " << DeqRate << endl;
    if((queueLength + packet->getBitLength() > (0.9 * maxSize)) && !congested){
        congestedNum[switchid][priority] += 1;
        congested = true;
    }else if(queueLength + packet->getBitLength() <= (0.9 * maxSize) && congestedNum[switchid][priority] > 0 && congested){
        congestedNum[switchid][priority] -= 1;
        congested = false;
    }

    if (queueLength - dataCapacity.get() + packet->getBitLength() >  maxSize){
        EV << "it's false" <<endl;
        return false; // drop
    }
    else{
        sharedBuffer[switchid] = RemainingBufferSize - b(packet->getBitLength());
        return true;
    }
}

} // namespace inet

