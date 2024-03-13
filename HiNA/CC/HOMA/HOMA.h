// Copyright (C)
/*
 * Developed by Lizihan
 * Begin at 2023/3/2
*/

#ifndef INET_HINA_CC_HOMA_HOMA_H
#define INET_HINA_CC_HOMA_HOMA_H

#include "../ccheaders.h"

using namespace std;

namespace inet {

class HOMA : public TransportProtocolBase
{
  protected:
    enum ReceiverState
    {
        GRANT_STOP,
        GRANT_SENDING
    };

    // states for self-scheduling
    enum SelfpckKindValues {
        SENDSCHEDULE,
        SENDUNSCHEDULE,
        SENDBUSY,
        SENDRESEND,
        SENDSTOP,
        TIMEOUT
    };

    //���շ�����Ϣ��
    struct receiver_flowinfo{
        L3Address destAddr; //Ŀ�ĵ�ַ
        int flowid; //��ID
        bool if_get[10000]; //���Ƿ��յ�״̬��
        int now_received_data_seq; //��ǰ�յ��İ����к�
        long now_send_grt_seq; //��ǰ���͵�GRANT���к�
        ReceiverState RcvState;
        int SenderPriority = -1;
        TimerMsg *sendresend;
        TimerMsg *timeout;
    };

    //���ͷ�����Ϣ��
    struct sender_flowinfo{
        L3Address destAddr; //Ŀ�ĵ�ַ
        int pckseq; //��ǰ���͵İ����к�
        int flowid; //��ID
        int srcPort; //Դ�˿ں�
        int destPort; //Ŀ�Ķ˿ں�
        CrcMode crcMode; //ѭ��������ģʽ
        uint16_t crc; //ѭ��������
        simtime_t cretime; //������ʱ��
        int64_t flowsize; //����С

        int64_t sendRtt; //�ѷ��͵�Unscheduled���ֳ���
        int64_t unsSize; //Unscheduled�ܲ��ֳ���
        int64_t sSize; //schedule�����ܳ���
        uint16_t unscheduledPrio; //unscheduled����ʹ�õ����ȼ�
    };


    // configuration for .ned file
    bool activate;
    simtime_t stopTime;
    double linkspeed;
    simtime_t baseRTT;
    int64_t max_pck_size;

    L3Address srcAddr;
    int rttbytes = 0;
    simtime_t timeout = 0.00002;
    simtime_t last_creation_time = 0;
    int lastflowid = -1;
    bool priorityUse[8] = {0};
    uint32_t maxSchedPktDataBytes;
    std::vector<uint32_t> prioCutOffs;
    std::vector<uint32_t> eUnschedPrioCutoff;
    // judging which destination the credit from
    L3Address sender_srcAddr;         // The source address of sender
    L3Address receiver_srcAddr;         // The source address of receiver
    L3Address next_destAddr;
    short next_schePrio;


    cGate *outGate;
    cGate *inGate;
    cGate *upGate;
    cGate *downGate;
    const char *packetName = "HomaData";


    //���ͷ�����
    std::map<uint32_t, sender_flowinfo> sender_flowMap;

    //���շ�����
    std::map<uint32_t, receiver_flowinfo> receiver_flowMap;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual ~HOMA() {
        for(auto i = receiver_flowMap.begin();i != receiver_flowMap.end();i++){
            cancelEvent(i->second.sendresend);
            delete i->second.sendresend;
            cancelEvent(i->second.timeout);
            delete i->second.timeout;
        }
    }

    /**
     * Should be redefined to send out the packet; e.g. <tt>send(pck,"out")</tt>.
     */
    virtual void sendDown(Packet *pck);
    virtual void sendUp(Packet *pck);

    virtual void send_resend(uint32_t flowid, long seq);
    virtual void send_unschedule(uint32_t flowid);

    virtual void send_grant(uint32_t flowid);
    virtual void send_busy( uint32_t flowid, long seq);

    virtual void time_out(uint32_t flowid);
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
    virtual void receive_grant(Packet *pck);
    virtual void receive_resend(Packet *pck);
    virtual void receive_busy(Packet *pck);
    virtual void receive_scheduledata(Packet *pck);
    virtual void receive_unscheduledata(Packet *pck);

    virtual void finish() override;

    // ILifeCycle:
    virtual void handleStartOperation(LifecycleOperation *operation) override{};
    virtual void handleStopOperation(LifecycleOperation *operation) override{};
    virtual void handleCrashOperation(LifecycleOperation *operation) override{};

    virtual uint16_t getMesgPrio(uint32_t msgSize);
    virtual void setPrioCutOffs();
};
} // namespace inet

#endif // ifndef __INET_HOMA_H

