/*
 * HiEthernetEncaplation.h
 *
 *  Created on: 2023��10��20��
 *      Author: ergeng2001
 */

//
// Copyright (C) 2003 Andras Varga; CTIE, Monash University, Australia
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#ifndef __INET_HIETHERNETENCAPSULATION_H
#define __INET_HIETHERNETENCAPSULATION_H

#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/lifecycle/OperationalBase.h"
#include "inet/common/packet/Message.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/common/FcsMode_m.h"
#include "inet/linklayer/ethernet/common/Ethernet.h"
#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "inet/networklayer/common/NetworkInterface.h"
#include "inet/linklayer/ethernet/basic/EthernetEncapsulation.h"



#include<map>
namespace inet {

/**
 * Performs Ethernet II encapsulation/decapsulation. More info in the NED file.
 */
class INET_API HiEthernetEncapsulation : public EthernetEncapsulation
{
protected:
  std::set<const Protocol *> upperProtocols; // where to send packets after decapsulation

  protected:
    FcsMode fcsMode = FCS_MODE_UNDEFINED;
    int seqNum;

    // statistics
    long totalFromHigherLayer; // total number of packets received from higher layer
    long totalFromMAC; // total number of frames received from MAC
    long totalPauseSent; // total number of PAUSE frames sent
    static simsignal_t encapPkSignal;
    static simsignal_t decapPkSignal;
    static simsignal_t pauseSentSignal;
    NetworkInterface *networkInterface = nullptr;


    std::map<uint64_t,uint32_t> flow_upstreamQueue;
    typedef std::map<uint64_t,uint32_t>::iterator flowQueuePair;

    //<flowid,upstreamQueueid>

    //һ��packet��mac������
    //1��Ϊ���ϴ����packet��ȥ��BFCHeader�飻
    //2��ʶ��packetЯ����flowid�ţ��Ƿ���map<>�д���
    //3�����flowid���ڣ��������¼�¼һ�飻
    //4�����flowid�����ڣ���flowid���뵽map�У�
    //5������map��ȥ��packetͷ����BFCHeader�飻
    //6�����������ݰ����ϲ㴫�䡣



    //����packet��ip�����´���Queue����
    //1��Ϊ���´�������ݰ�������BFCHeader�飻
    //2��ʶ��ǰ��sender����switch
    //3�������sender������setUpstreamQueueid ==0;
    //4�������switch���ҵ���packet��flowid;
    //5����map�в���flowid��Ӧ��upstreamQueueid����������뵽BFCHeader��;
    //6����map���ҵ�flowid��ɾ������Ӧ����Ŀ������map������������
    //7��packet�������´���Queue�У���queue������У������packetû�д���XOFF�����������䵽receiver;
    //8�������packet����XOFF�����ɶ��в�����ҵ�BFCHeader�У�upstreamQueueid����Ϣ;
    //9���������XOFF������֪�����ζ˿ڣ���ν���ζ˿ڣ����Ǹ�packet��˿������ӵ�����switch�ĳ��˿ڡ�

    //for BFC UPstreamQueueID
/*
    struct Router_upstreamQueueID{
        std::string routerName;
        uint32_t upstreamQueueID = 0;
    };
*/

/*
    std::map<uint32_t,uint32_t> Router_upstreamID;//first for routerid; second for upstreamQueueID
    std::map<uint32_t,uint32_t>::iterator iter_routerid;
*/

/*
    std::multimap<uint32_t,Router_upstreamQueueID> UpstreamQueueID_Map;// first for flowid; second for struct: routerID and upstreamQueueID;
    typedef std::multimap<uint32_t,Router_upstreamQueueID>::iterator iter_rflowid;
*/

    struct Socket {
        int socketId = -1;
        int interfaceId = -1;
        MacAddress localAddress;
        MacAddress remoteAddress;
        const Protocol *protocol = nullptr;
        bool steal = false;

        Socket(int socketId) : socketId(socketId) {}
        bool matches(Packet *packet, int ifaceId, const Ptr<const EthernetMacHeader>& ethernetMacHeader);
    };

    friend std::ostream& operator<<(std::ostream& o, const Socket& t);
    std::map<int, Socket *> socketIdToSocketMap;
    bool anyUpperProtocols = false;

  protected:
    virtual ~HiEthernetEncapsulation();
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *message) override;

    virtual void handleRegisterService(const Protocol& protocol, cGate *gate, ServicePrimitive servicePrimitive) override;
    virtual void handleRegisterProtocol(const Protocol& protocol, cGate *gate, ServicePrimitive servicePrimitive) override;
    virtual void handleRegisterAnyProtocol(cGate *gate, ServicePrimitive servicePrimitive) override;

    virtual void processCommandFromHigherLayer(Request *msg) override;
    virtual void processPacketFromHigherLayer(Packet *msg) override;
    virtual void processPacketFromMac(Packet *packet) override;
    virtual void handleSendPause(cMessage *msg) override;

    virtual void refreshDisplay() const override;

    // for lifecycle:
    virtual bool isInitializeStage(int stage) const override { return stage == INITSTAGE_LINK_LAYER; }
    virtual bool isModuleStartStage(int stage) const override { return stage == ModuleStartOperation::STAGE_LINK_LAYER; }
    virtual bool isModuleStopStage(int stage) const override { return stage == ModuleStopOperation::STAGE_LINK_LAYER; }
    virtual void clearSockets() override;
    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;
};

} // namespace inet

#endif




