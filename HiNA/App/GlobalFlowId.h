/*
 * Flowid.h
 *
 *  Created on: 2023Äê6ÔÂ23ÈÕ
 *      Author: 10202
 */

#ifndef INET_HINA_APP_GLOBALFLOWID_H_
#define INET_HINA_APP_GLOBALFLOWID_H_

#include "inet/common/INETDefs.h"

namespace inet{

class GlobalFlowId : public cSimpleModule
{
    public:
    static uint64_t flowid;

    void flowidadd(){
        flowid++;
    }
    uint64_t getflowid(){
        return flowid;
    }
};

}





#endif /* INET_HINA_APP_GLOBALFLOWID_H_ */
