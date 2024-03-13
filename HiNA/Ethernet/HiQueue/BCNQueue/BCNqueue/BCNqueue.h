/*
 * BCNQueue.h
 *
 *  Created on: 2023年10月22日
 *      Author: ergeng2001
 */



#ifndef INET_HINA_ETHERNET_HIQUEUE_BCNQUEUE_BCNQUEUE_BCNQUEUE_H_
#define INET_HINA_ETHERNET_HIQUEUE_BCNQUEUE_BCNQUEUE_BCNQUEUE_H_

#include <vector>
#include <algorithm>

#include "../../REDPFCQueue/REDPFCQueue.h"

#include "../../../../Messages/BfcFrame/EthernetBfcFrame_m.h"
#include "../../../../Messages/BfcCNP/EthernetBfcCNP_m.h"
#include "../../../../Messages/BFCHeader/BFCHeader_m.h"
#include "../../../../Messages/BFCHeader/BFCHeaderSerializer.h"
#include "../../../../Messages/BFCTag/isPause_m.h"

#include "../../../../../networklayer/ipv4/Ipv4Header_m.h"

#include "../../../../../common/packet/chunk/Chunk.h"
#include "../../../../../common/packet/Packet.h"


namespace inet {
using namespace inet::queueing;

class INET_API BCNqueue : public PacketQueueBase, public IPacketBuffer::ICallback
{
  public:
    int packetCapacity = -1;
    b dataCapacity = b(-1);
    static b sharedBuffer[100][100];
    b headroom;
    double alpha;
    uint64_t maxSize;

    IActivePacketSource *producer = nullptr;
    IActivePacketSink *collector = nullptr;

    cPacketQueue queue;
//    IPacketBuffer *buffer = nullptr;

    IPacketDropperFunction *packetDropperFunction = nullptr;
    IPacketComparatorFunction *packetComparatorFunction = nullptr;

    bool useBfc;


    //定义multimap，存储每个上游端口的唯一队列，如果能够查找到指定端口的指定队列，说明是pause；
    //发送resume之后，再从multimap中删除该队列
    std::multimap<int,int> ispaused;//first is eth_id ;second is upQueue_id;
    typedef std::multimap<int,int>::iterator iter_ef;

    //paused vector 用于该队列中，某个flowid是否被暂停，保证每个队列只对该队列的某个流仅发送一次暂停帧；
    std::vector<uint64_t> isPausedFlow;//记录flowid 被降速；降速和升速都仅进行一次；

    int XON;
    int XOFF;
    //int priority;
    int numb;
    int Kmax;
    int Kmin;
    double Pmax;
    bool useEcn;
    bool markNext = false;
    double count = NaN;
    int switchid;

    //for send resume packet
    bool isPausedPck;
    bool lastPauseID;

    enum RedResult { RANDOMLY_ABOVE_LIMIT, RANDOMLY_BELOW_LIMIT, ABOVE_MAX_LIMIT, BELOW_MIN_LIMIT };
    mutable RedResult lastResult;

    cOutVector queuelengthVector;//由于加了共享缓存，用于计算queuelength
    cOutVector sharedBufferVector;//由于加了共享缓存，用于计算queuelength

    //for statistic
    int sendcnpdel = 0;
    int sendcnpspe = 0;



  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;

    virtual IPacketDropperFunction *createDropperFunction(const char *dropperClass) const;
    virtual IPacketComparatorFunction *createComparatorFunction(const char *comparatorClass) const;
    virtual RedResult doRandomEarlyDetection(const Packet *packet);
    virtual bool isOverloaded() const;

  public:
    static simsignal_t bfcPausedSignal;
    static simsignal_t bfcResumeSignal;

    static simsignal_t bfcDecelerationSignal;
    static simsignal_t bfcSpeedupSignal;


    virtual ~BCNqueue() { delete packetDropperFunction; }

    virtual int getMaxNumPackets() const override { return packetCapacity; }
    virtual int getNumPackets() const override;

    virtual b getMaxTotalLength() const override { return dataCapacity; }
    virtual b getTotalLength() const override { return b(queue.getBitLength()); }

    virtual bool isEmpty() const override { return getNumPackets() == 0; }
    virtual Packet *getPacket(int index) const override;
    virtual void removePacket(Packet *packet) override;
    virtual void removeAllPackets() override;

    virtual bool supportsPacketPushing(cGate *gate) const override { return inputGate == gate; }
    virtual bool canPushSomePacket(cGate *gate) const override;
    virtual bool canPushPacket(Packet *packet, cGate *gate) const override;
    virtual void pushPacket(Packet *packet, cGate *gate) override;

    virtual bool supportsPacketPulling(cGate *gate) const override { return outputGate == gate; }
    virtual bool canPullSomePacket(cGate *gate) const override { return !isEmpty(); }
    virtual Packet *canPullPacket(cGate *gate) const override { return !isEmpty() ? getPacket(0) : nullptr; }
    virtual Packet *pullPacket(cGate *gate) override;

    virtual void handlePacketRemoved(Packet *packet) override;

    virtual void pfcpaused() {}
    virtual void pfcresumed() {}
    virtual bool BufferManagement(cMessage *msg);
    virtual void finish() override;
};

} // namespace inet



#endif /* INET_HINA_ETHERNET_HIQUEUE_BFCQUEUE_BFCQUEUE_H_ */
