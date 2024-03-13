/*
 * BFCqueue.h
 *
 *  Created on: 2023��4��27��
 *      Author: ergeng2001
 */

#ifndef INET_HINA_ETHERNET_HIQUEUE_BFCQUEUE_BFCQUEUE_H_
#define INET_HINA_ETHERNET_HIQUEUE_BFCQUEUE_BFCQUEUE_H_

#include <vector>
#include <algorithm>

#include "../../REDPFCQueue/REDPFCQueue.h"

#include "../../../../Messages/BfcFrame/EthernetBfcFrame_m.h"
#include "../../../../Messages/BFCHeader/BFCHeader_m.h"
#include "../../../../Messages/BFCHeader/BFCHeaderSerializer.h"
#include "../../../../Messages/BFCTag/isPause_m.h"

#include "../../../../../networklayer/ipv4/Ipv4Header_m.h"

#include "../../../../../common/packet/chunk/Chunk.h"
#include "../../../../../common/packet/Packet.h"


namespace inet {
using namespace inet::queueing;

class INET_API BFCqueue : public PacketQueueBase, public IPacketBuffer::ICallback
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

    IPacketDropperFunction *packetDropperFunction = nullptr;
    IPacketComparatorFunction *packetComparatorFunction = nullptr;

    bool useBfc;

    //1������Multimap<eth,upstreamQid>����ǰÿ��queue���洢���ζ˿ڵ���ͣ���У�
    //2������PAUSE֡������<eth,upstreamQid>����Multimap��
    //3����isPause���ݰ��ų����У�����RESUME֡���ָ����ζ��У�
    //4������RESUME�󣬽�<eth,upstreamQid>��Multimap��ɾ����

    std::multimap<int,int> ispaused;
    typedef std::multimap<int,int>::iterator iter_ef;

    //1��vector isPausedFlow ���ڴ洢��ǰ�����ѷ��͹�PAUSE֡��flowid��
    //2����isPause���ݰ��뿪���У�����RESUME֡��
    //3������flowid�� vector isPausedFlow��ɾ����

    std::vector<uint64_t> isPausedFlow;
    int XON;
    int XOFF;
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

    cOutVector queuelengthVector;//���ڼ��˹����棬���ڼ���queuelength
    cOutVector sharedBufferVector;//���ڼ��˹����棬���ڼ���queuelength


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
    virtual ~BFCqueue() { delete packetDropperFunction; }

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

    virtual void bfcpaused() {}
    virtual void bfcresumed() {}
    virtual bool BufferManagement(cMessage *msg);

};

} // namespace inet

#endif /* INET_HINA_ETHERNET_HIQUEUE_BFCQUEUE_BFCQUEUE_H_ */
