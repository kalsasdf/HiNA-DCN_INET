//
// Generated file, do not edit! Created by opp_msgtool 6.0 from inet/HiNA/Messages/HiTag/HiTag.msg.
//

#ifndef __INET_INET_HINA_MESSAGES_HITAG_HITAG_M_H
#define __INET_INET_HINA_MESSAGES_HITAG_HITAG_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// opp_msgtool version check
#define MSGC_VERSION 0x0600
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgtool: 'make clean' should help.
#endif

// dll export symbol
#ifndef INET_API
#  if defined(INET_EXPORT)
#    define INET_API  OPP_DLLEXPORT
#  elif defined(INET_IMPORT)
#    define INET_API  OPP_DLLIMPORT
#  else
#    define INET_API
#  endif
#endif


namespace inet {

class HiTag;

}  // namespace inet

#include "inet/common/INETDefs_m.h" // import inet.common.INETDefs

#include "inet/common/TagBase_m.h" // import inet.common.TagBase

#include "inet/common/Units_m.h" // import inet.common.Units


namespace inet {

/**
 * Class generated from <tt>inet\HiNA\Messages\HiTag\HiTag.msg:12</tt> by opp_msgtool.
 * <pre>
 * class HiTag extends TagBase
 * {
 *     uint32_t Priority = -1;//防止没设置优先级的包进入优先级队列
 *     uint64_t PacketId;
 *     uint64_t FirstRttPcks;
 *     uint32_t FlowId;
 *     uint64_t FlowSize;
 *     uint64_t PacketSize;
 *     bool reverse = false;//信用协议反向路由用
 *     simtime_t creationtime;
 *     bool isLastPck = false;//for TIMELY
 * 
 *     //for PFC
 *     int16_t op;
 *     int16_t interfaceId;
 * }
 * </pre>
 */
class INET_API HiTag : public ::inet::TagBase
{
  protected:
    uint32_t Priority = -1;
    uint64_t PacketId = 0;
    uint64_t FirstRttPcks = 0;
    uint32_t FlowId = 0;
    uint64_t FlowSize = 0;
    uint64_t PacketSize = 0;
    bool reverse = false;
    ::omnetpp::simtime_t creationtime = SIMTIME_ZERO;
    bool isLastPck_ = false;
    int16_t op = 0;
    int16_t interfaceId = 0;

  private:
    void copy(const HiTag& other);

  protected:
    bool operator==(const HiTag&) = delete;

  public:
    HiTag();
    HiTag(const HiTag& other);
    virtual ~HiTag();
    HiTag& operator=(const HiTag& other);
    virtual HiTag *dup() const override {return new HiTag(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    virtual uint32_t getPriority() const;
    virtual void setPriority(uint32_t Priority);

    virtual uint64_t getPacketId() const;
    virtual void setPacketId(uint64_t PacketId);

    virtual uint64_t getFirstRttPcks() const;
    virtual void setFirstRttPcks(uint64_t FirstRttPcks);

    virtual uint32_t getFlowId() const;
    virtual void setFlowId(uint32_t FlowId);

    virtual uint64_t getFlowSize() const;
    virtual void setFlowSize(uint64_t FlowSize);

    virtual uint64_t getPacketSize() const;
    virtual void setPacketSize(uint64_t PacketSize);

    virtual bool getReverse() const;
    virtual void setReverse(bool reverse);

    virtual ::omnetpp::simtime_t getCreationtime() const;
    virtual void setCreationtime(::omnetpp::simtime_t creationtime);

    virtual bool isLastPck() const;
    virtual void setIsLastPck(bool isLastPck);

    virtual int16_t getOp() const;
    virtual void setOp(int16_t op);

    virtual int16_t getInterfaceId() const;
    virtual void setInterfaceId(int16_t interfaceId);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const HiTag& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, HiTag& obj) {obj.parsimUnpack(b);}


}  // namespace inet


namespace omnetpp {

template<> inline inet::HiTag *fromAnyPtr(any_ptr ptr) { return check_and_cast<inet::HiTag*>(ptr.get<cObject>()); }

}  // namespace omnetpp

#endif // ifndef __INET_INET_HINA_MESSAGES_HITAG_HITAG_M_H

