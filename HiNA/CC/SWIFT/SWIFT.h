/*
 * SWIFT.h
 *
 *  Created on: 20230205
 *      Author: luca
 */

#ifndef INET__SWIFT_H_
#define INET__SWIFT_H_

#include "../ccheaders.h"
#include "inet/HiNA/Messages/SWIFT/SACK_m.h"

using namespace std;

namespace inet {


class SWIFT : public TransportProtocolBase
{
protected:
    enum SenderState{
        SENDING,
        PAUSING,
        STOPPING
    };
    SenderState SenderState=STOPPING;

    bool FAST_RECOVERY = false;

    enum SelfpckKindValues {
        SENDDATA,
        TIMEOUT
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

    struct receiver_info{
        uint rcv_nxt = 0;
        std::list<SackItem> sacks_array;
        vector<uint> acksequence;
    };

    std::list<uint> sacks_array_snd;
    TimerMsg *senddata = nullptr;
    TimerMsg *timeout = nullptr;
    // configuration for .ned file
    bool activate;
    simtime_t stopTime;
    double linkspeed;
    int64_t max_pck_size;
    simtime_t baseRTT;
    double ai;
    double alpha;
    double beta;
    double max_mdf;
    double hop_scaler;
    int hops;
    double fs_min_cwnd;
    double fs_max_cwnd;

    simtime_t RTT_S = 0;
    simtime_t RTT_D = 0;
    simtime_t RTO = 0.1;
    simtime_t t_last_decrease = 0;
    double RTO_alpha = 0.125;
    double RTO_beta = 0.25;
    double min_cwnd = 0.0001;
    int num_acked = 0;
    int num_dupack = 0;
    uint64_t packetid = 0;
    uint64_t nxtSendpacketid = 0;
    uint64_t snd_una = 0;
    simtime_t currentRTT = 0;
    simtime_t lastRTT = 0;
    double cwnd_prev = 0;
    int retransmit_cnt = 0;
    int RETX_RESET_THRESHOLD = 5;
    simtime_t pacing_delay = 0;
    double pre_snd = 0;
    bool can_decrease = true;

    double fs_range;
    double snd_cwnd;
    double max_cwnd;

    cOutVector currentRTTVector;
    cOutVector targetVector;
    cOutVector cwndVector;
    cOutVector currateVector;
    int bitlength = 0;
    simtime_t maxInterval;
    simtime_t lasttime = 0;
    double currentRate;
    int timeout_num = 0;

    const char *packetName = "SWIFTData";
    cGate *lowerOutGate;
    cGate *lowerInGate;
    cGate *upperOutGate;
    cGate *upperInGate;

    // structures for data, credit and informations
    // The destinations addresses of the flows at sender, The flow information at sender
    std::map<uint32_t, sender_packetinfo> sender_packetMap;

    std::map<L3Address, receiver_info> receiver_Map;

protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual ~SWIFT() {cancelEvent(senddata); delete senddata;
    cancelEvent(timeout); delete timeout;}
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

    virtual void time_out();
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




#endif /* INET_SWIFT_H_ */
