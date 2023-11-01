//
// This program is property of its copyright holder. All rights reserved.
//

import inet.common.INETDefs;
import inet.common.TagBase;
import inet.common.Units;
import inet.networklayer.common.L3Address;

namespace inet;


class HiTag extends TagBase
{
    uint32_t Priority=-1;//防止没设置优先级的包进入优先级队列
    uint64_t PacketId;
    uint64_t FirstRttPcks;
    uint32_t FlowId;
    uint64_t FlowSize;
    uint64_t PacketSize;
    bool reverse = false;//信用协议反向路由用
    simtime_t creationtime;
    bool isLastPck = false;
    uint32_t senderPriority=-1;//for HOMA
    
    //for PFC
    int16_t op;
    int16_t interfaceId;
    
    //for BCN 
    bool isDeceleration;
    bool isSpeedup;
    inet::L3Address destAddr;
    inet::L3Address srcAddr;
    int16_t local_interfaceId; //记录本地mac端口
    int bfccnpECN;//用于判断发送的cnp包是否处于拥塞交换机处
    
    //for BFC
    uint32_t queueID; //定向在app，flowid % queueNum 写死。
    uint32_t upstreamQueueID ;
    bool ispausePck;
    
}