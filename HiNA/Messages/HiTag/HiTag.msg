//
// This program is property of its copyright holder. All rights reserved.
//

import inet.common.INETDefs;
import inet.common.TagBase;
import inet.common.Units;

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
    bool isLastPck = false;//for TIMELY
    
    //for PFC
    int16_t op;
    int16_t interfaceId;
}