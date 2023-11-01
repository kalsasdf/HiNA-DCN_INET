/*
 * HiEthernetEncaplation.h
 *
 *  Created on: 2023年10月20日
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

    //一、packet从mac传上来
    //1、为向上传输的packet，去掉BFCHeader块；
    //2、识别packet携带的flowid号，是否在map<>中存在
    //3、如果flowid存在，则不再重新记录一遍；
    //4、如果flowid不存在，则将flowid存入到map中；
    //5、存入map后，去掉packet头部的BFCHeader块；
    //6、继续将数据包向上层传输。



    //二、packet从ip层向下传到Queue队列
    //1、为向下传输的数据包，加上BFCHeader块；
    //2、识别当前是sender还是switch
    //3、如果是sender，设置setUpstreamQueueid ==0;
    //4、如果是switch，找到该packet的flowid;
    //5、在map中查找flowid对应的upstreamQueueid，并将其存入到BFCHeader中;
    //6、在map中找到flowid后，删除掉对应的条目，减轻map的容量负担；
    //7、packet继续向下传到Queue中，在queue中入队列，如果该packet没有触发XOFF，则继续向后传输到receiver;
    //8、如果该packet触发XOFF，则由队列拆包，找到BFCHeader中，upstreamQueueid的信息;
    //9、如果触发XOFF，并且知道上游端口；所谓上游端口，就是该packet入端口相连接的上游switch的出端口。

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




