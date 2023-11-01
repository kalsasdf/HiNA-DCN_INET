/*
 * BCNClassifier.cc
 *
 *  Created on: 2023��10��23��
 *      Author: ergeng2001
 */

//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "BCNScheduler.h"

namespace inet {

Define_Module(BCNScheduler);

BCNScheduler::~BCNScheduler()
{
    delete[] weights02;
    delete[] buckets02;
}

void BCNScheduler::initialize(int stage)
{
    PacketSchedulerBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {

        weights02 = new unsigned int[providers.size()];
        buckets02 = new unsigned int[providers.size()];

        cStringTokenizer tokenizer(par("weights02"));
        size_t i;
        for (i = 0; i < providers.size() && tokenizer.hasMoreTokens(); ++i)
             buckets02[i] = weights02[i] = utils::atoul(tokenizer.nextToken());

        if (i < providers.size())
             throw cRuntimeError("Too few values given in the weights02 parameter.");
        if (tokenizer.hasMoreTokens())
             throw cRuntimeError("Too many values given in the weights02 parameter.");


        for (auto provider : providers)
            collections.push_back(dynamic_cast<BCNqueue *>(provider));

        numInputs = gateSize("in");

        cModule *radioModule = getParentModule()->getParentModule();EV<<"parentmodule = "<<radioModule<<endl;
        radioModule->subscribe(BCNMac::bfcPausedFrame,this);
        radioModule->subscribe(BCNMac::bfcResumeFrame,this);
        for (int i = 0; i < numInputs; ++i) {
            ispaused[i] = 0;
        }
    }
}

int BCNScheduler::getNumPackets() const
{
    int size = 0;
    for (auto collection : collections)
        if (collection != nullptr)
            size += collection->getNumPackets();
        else
            return -1;
    return size;
}

b BCNScheduler::getTotalLength() const
{
    b totalLength(0);
    for (auto collection : collections)
        if (collection != nullptr)
            totalLength += collection->getTotalLength();
        else
            return b(-1);
    return totalLength;
}

Packet *BCNScheduler::getPacket(int index) const //�����ǵõ�ָ��index�İ�������ָ��index�Ķ���
{
    int origIndex = index;

    for (auto collection : collections) {
        auto numPackets = collection->getNumPackets();
        if (index < numPackets)//��¼last packet ��flowid��index; �������index ��ȡ���İ���flowid == last flowid
            //�������ϴε�last_index��ȡ��; ���򣬸���last_index;
        {
            return collection->getPacket(index);

            /*
            Packet* onepacket = collection->getPacket(index);
            auto& Hipacket = onepacket->peekAtFront<ByteCountChunk>();
            for(auto& region:onepacket->peekData()->getAllTags<HiTag>()){
                flowid = region.getTag()->getFlowId();
            }

            if(last_flowid != flowid){//��Ҫ����index �� flowid
                last_flowid = flowid;
                last_index = index;
                return collection->getPacket(index);
            }else{
                return collection->getPacket(last_index);
            }
            */
        }

        else
            index -= numPackets;
    }
    throw cRuntimeError("Index %i out of range", origIndex);
}

void BCNScheduler::removePacket(Packet *packet)
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

void BCNScheduler::removeAllPackets()
{
    Enter_Method("removeAllPackets");
    for (auto collection : collections)
        collection->removeAllPackets();
}

bool BCNScheduler::canPullSomePacket(cGate *gate) const
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


int BCNScheduler::schedulePacket()
{
    int firstWeighted = -1;
    int firstNonWeighted = -1;
    uint64_t flowid;

    for (size_t i = 0; i < collections.size(); i++) {

        int inputIndex = getInputGateIndex(i);
        if (inputIndex == inProgressGateIndex || providers[inputIndex]->canPullSomePacket(inputGates[inputIndex]->getPathStartGate()))
        {
            EV<<"i = "<< i <<"; buckets02[i] =  "<<buckets02[i]<<endl;
            if(buckets02[i] >0 ){//���ﲻ����
                buckets02[i]--;
                return inputIndex;

            }else if (firstWeighted == -1 && weights02[i] > 0)
                    firstWeighted = (int)i;
                else if (firstNonWeighted == -1 && weights02[i] == 0)
                    firstNonWeighted = (int)i;
         }
    }
    if (firstWeighted != -1){
        for (size_t i = 0; i < providers.size(); ++i)
            buckets02[i] = weights02[i];
         buckets02[firstWeighted]--;
         //EV<<"restart from first weighted scheduler packet in num = "<< firstWeighted;
         return firstWeighted;
     }

    if (firstNonWeighted != -1){
        //EV<<"restart from first nonweighted scheduler packet in num = "<< firstNonWeighted;
         return firstNonWeighted;
    }
    //ѡ����

    return -1;
}


Packet *BCNScheduler::pullPacket(cGate *gate)//
{
    Enter_Method("pullPacket");
    checkPacketStreaming(nullptr);
    //ÿ����ѯ����
    int index = callSchedulePacket();
    auto packet = providers[index]->pullPacket(inputGates[index]->getPathStartGate());//outputGate;
    take(packet);
    EV_INFO << "Scheduling packet" << EV_FIELD(packet) << EV_ENDL;
    handlePacketProcessed(packet);
    emit(packetPulledSignal, packet);
    animatePullPacket(packet, outputGate);
    updateDisplayString();
    return packet;
    //ÿ����ѯ����

    /*
    //ÿ����ѯ����
    if(last_index == -1){//��һ�ε���packet����¼��һ��last_index��last_flowid
        last_index = callSchedulePacket(); //last_index������¼������
        EV<<"first index = callSchedulePacket() : "<< last_index <<endl;
        auto packet = providers[last_index]->pullPacket(inputGates[last_index]->getPathStartGate());
        auto& HiPacket = packet->peekAtFront<ByteCountChunk>();
        for(auto& region:packet->peekData()->getAllTags<HiTag>()){
            last_flowid = region.getTag()->getFlowId();
        }
            take(packet);
            EV_INFO << "First Scheduling packet" << EV_FIELD(packet) << EV_ENDL;
            EV<<"the first packet's flowid = "<<last_flowid <<endl;

            handlePacketProcessed(packet);
            emit(packetPulledSignal, packet);
            animatePullPacket(packet, outputGate);
            updateDisplayString();
            return packet;

    }else{//��Ȼ����һ�ε�last_gate
            int this_flowid;
            auto packet = providers[last_index]->pullPacket(inputGates[last_index]->getPathStartGate());
            if (std::string(packet->getFullName()).find("DCQCN") != std::string::npos){//��DCQCN��
            auto& HiPacket = packet->peekAtFront<ByteCountChunk>();
            for(auto& region:packet->peekData()->getAllTags<HiTag>()){
                this_flowid = region.getTag()->getFlowId();
            }

            if(this_flowid != last_flowid){//���������һ�ε�last_index���õ��İ���last_flow���������ٸ���index���������¼�¼last_flowid
                last_index = callSchedulePacket();
                auto packet = providers[last_index]->pullPacket(inputGates[last_index]->getPathStartGate());
                EV<<"this flowid (had changed) = "<<this_flowid<<endl;
                EV<<"last_flowid = "<<last_flowid<<endl;
                last_flowid = this_flowid;
                take(packet);
                EV_INFO << "Scheduling (new index) packet" << EV_FIELD(packet) << EV_ENDL;

                handlePacketProcessed(packet);
                emit(packetPulledSignal, packet);
                animatePullPacket(packet, outputGate);
                updateDisplayString();
                return packet;
            }else{//������һ�ε�last_index���������ϸ������е��ð�
            //������õ���this_flowid��last_flowidһ�£��ͷ��ص�ǰ���еĸ�packet��
                take(packet);
                EV_INFO << "Scheduling (last index) packet" << EV_FIELD(packet) << EV_ENDL;
                EV<<"schedule the last_index : "<<last_index<<endl;
                EV<<"this flowid (const last_flowid)= "<<this_flowid<<endl;
                EV<<"last_flowid = "<<last_flowid<<endl;

                handlePacketProcessed(packet);
                emit(packetPulledSignal, packet);
                animatePullPacket(packet, outputGate);
                updateDisplayString();
                return packet;
            }
            }else{//����DCQCN������sefMsg;ֱ�ӷ��������
                        take(packet);
                        EV_INFO << "Scheduling packet sefMsg" << EV_FIELD(packet) << EV_ENDL;

                        handlePacketProcessed(packet);
                        emit(packetPulledSignal, packet);
                        animatePullPacket(packet, outputGate);
                        updateDisplayString();
                        return packet;
            }

    }//ÿ����ѯ����
    */

}


void BCNScheduler::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    if(signalID==BCNMac::bfcPausedFrame||signalID==BCNMac::bfcResumeFrame){
        EV<<"BCNScheduler::receiveSignal(), receive bfc frame"<<endl;
        processbfcframe(obj);
    }
}

void BCNScheduler::processbfcframe(cObject *obj){
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




