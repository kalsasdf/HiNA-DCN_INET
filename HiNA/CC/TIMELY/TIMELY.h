/*
 * TIMELY.h
 *
 *  Created on: 20230205
 *      Author: luca
 */

#ifndef INET_ANGRYDUDU_TIMELY_TIMELY_H_
#define INET_ANGRYDUDU_TIMELY_TIMELY_H_

#include "../ccheaders.h"

using namespace std;

namespace inet {


class TIMELY : public TransportProtocolBase
{
protected:
    enum SenderState{
        SENDING,
        STOPPING
    };
    SenderState SenderState=STOPPING;

    enum SelfpckKindValues {
        SENDDATA,
    };

    struct sender_flowinfo{
        L3Address destAddr;
        int flowid;
        int pckseq;
        int srcPort;
        int destPort;
        CrcMode crcMode;
        uint16_t crc;
        uint32_t priority;
        simtime_t cretime;
        int64_t remainLength;
    };

    TimerMsg *senddata = nullptr;
    // configuration for .ned file
    bool activate;
    simtime_t stopTime;
    simtime_t minRTT;
    simtime_t Tlow;
    simtime_t Thigh;
    double linkspeed;
    double Rai;
    int64_t max_pck_size;
    double alpha;
    double beta;
    int TIMELYseg;

    double rtt_diff = 0;
    uint32_t Number = 1;
    simtime_t currentRTT = 0;
    simtime_t lastRTT = 0;
    double currentRate;
    int accumlength = 0;
    int segcount = 0;
    bool isLastPck = false;

    L3Address srcAddr;

    const char *packetName = "TIMELYData";
    cGate *lowerOutGate;
    cGate *lowerInGate;
    cGate *upperOutGate;
    cGate *upperInGate;

    // structures for data, credit and informations
    // The destinations addresses of the flows at sender, The flow information at sender
    std::map<uint32_t, sender_flowinfo> sender_flowMap;
    std::map<uint32_t, sender_flowinfo>::iterator iter;

protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual ~TIMELY() {cancelEvent(senddata); delete senddata;}
    /**
     * Should be redefined to send out/receive from the packet.
     */
    virtual void sendDown(Packet *pck);
    virtual void sendUp(Packet *pck);

    virtual void send_data();
    virtual void receive_data(Packet *pck);
    /*
     *  Receive ack and Timely Rate Update Algorithm
     */
    virtual void receive_ack(Packet *pck);
    /*
     * Process the Packet from upper/lower layer
     */
    virtual void processUpperpck(Packet *pck);
    virtual void processLowerpck(Packet *pck);

    virtual void finish() override;
    // ILifeCycle:
    virtual void handleStartOperation(LifecycleOperation *operation) override{};
    virtual void handleStopOperation(LifecycleOperation *operation) override{};
    virtual void handleCrashOperation(LifecycleOperation *operation) override{};
};

} // namespace inet




#endif /* INET_ANGRYDUDU_TIMELY_TIMELY_H_ */
