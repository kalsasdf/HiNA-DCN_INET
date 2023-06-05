// Copyright (C)
/*
 * Developed by Angrydudu-gy
 * Begin at 2020/04/28
*/
#ifndef __INET_HPCC_H
#define __INET_HPCC_H

#include "../ccheaders.h"
#include "inet/HiNA/Messages/HPCC/INTHeader_m.h"
using namespace std;

namespace inet {

class HPCC : public TransportProtocolBase//cSimpleModule
{
protected:
    // states for self-scheduling
    enum SenderState{
        SENDING,
        PAUSING,
        STOPPING,
    };
    SenderState SenderState=STOPPING;

    enum SelfpckKindValues {
        SENDDATA,
    };

    struct sender_packetinfo{
        L3Address srcAddr;//ip address
        L3Address destAddr;
        int flowid;
        int srcPort;
        int destPort;
        int length;
        CrcMode crcMode;
        uint16_t crc;
        uint32_t priority;
        simtime_t cretime;
        bool last = false;
    };

    TimerMsg *senddata = nullptr;
    // configuration for .ned file
    bool activate;
    simtime_t stopTime;
    double linkspeed;
    int64_t max_pck_size;
    int expectedFlows;// the expected maximum number of concurrent flows on a link
    simtime_t baseRTT;

    bool isFirstAck = true;
    uint64_t packetid = 0;
    uint64_t nxtSendpacketid = 0;
    double yita = 0.95;
    int maxstage = 5;
    int lastUpdateSeq = -1;//sequence number of ACK(wanted sequence at the receiver) that triggered the last update
    int incstage = 0;//the count of Addictive Increase

    double U=0;//the normalized total inflight bytes
    uint send_window;//parameter w
    uint csend_window;//parameter wc
    double currentRate;
    hopInf Last[10];
    int wint;//parameter wint
    int wai;//additive increase(AI) part to ensure fairness

    const char *packetName = "HPCCData";
    cGate *lowerOutGate;
    cGate *lowerInGate;
    cGate *upperOutGate;
    cGate *upperInGate;

    std::map<uint64_t, sender_packetinfo> sender_packetMap;

    std::map<L3Address, int> receiver_packetMap;

    simsignal_t Usignal;
    double linkrate;
    cOutVector linkrateVector;
    int queuelength;
    cOutVector queuelengthVector;

protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual ~HPCC() {cancelEvent(senddata); delete senddata;}

    virtual void sendDown(Packet *pck);
    virtual void sendUp(Packet *pck);

    virtual void send_data();

    /*
     * Process the Packet from upper layer
     */
    virtual void processUpperpck(Packet *pck);
    /*
     * Process the Packet from lower layer
     */
    virtual void processLowerpck(Packet *pck);
    /*
     * Receive XXX, and what should TODO next?
     */

    virtual void receive_data(Packet *pck);

    virtual double minval(b numa, b numb);//pick the minimal value

    virtual void receiveAck(Packet *pck);

    virtual void finish() override;
    // ILifeCycle:
    virtual void handleStartOperation(LifecycleOperation *operation) override{};
    virtual void handleStopOperation(LifecycleOperation *operation) override{};
    virtual void handleCrashOperation(LifecycleOperation *operation) override{};
};

} // namespace inet

#endif // ifndef __INET_HPCC_H

