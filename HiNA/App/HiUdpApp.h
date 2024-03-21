//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004,2011 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_HIUdpAPP_H
#define __INET_HIUdpAPP_H

#include <vector>
#include <fstream>

#include "GlobalFlowId.h"
#include "inet/HiNA/Messages/HiTag/HiTag_m.h"

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "inet/applications/base/ApplicationBase.h"
#include "inet/applications/base/ApplicationPacket_m.h"

#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

#include "inet/common/clock/ClockUserModuleMixin.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"


namespace inet {

/**
 * UDP application. See NED for more info.
 */
class INET_API HiUdpApp : public ClockUserModuleMixin<ApplicationBase>, public UdpSocket::ICallback
{
  protected:
    enum SelfMsgKinds { START = 1, SEND, STOP };

    struct Command {
        simtime_t tSend;
        long numBytes = 0;
        Command(simtime_t t, long n) { tSend = t; numBytes = n; }
    };
    typedef std::vector<Command> CommandVector;
    CommandVector commands;
    int commandIndex = -1;

    // parameters
    std::vector<L3Address> connectAddresses;
    std::vector<std::string> destAddressStr;
    int localPort = -1, connectPort = -1;
    simtime_t startTime=0;
    simtime_t stopTime=0;
    const char *packetName = nullptr;
    uint longflow=0;
    int AppPriority;
    const char *trafficMode = nullptr;
    double linkSpeed;
    double workLoad;
    bool Enablepoisson;

    uint messageLength=0;
    // state
    UdpSocket socket;
    cMessage *selfMsg = nullptr;

    // statistics
    int numSent = 0;
    int numReceived = 0;
    //static uint32_t flowid;    // counter for generating a global number for each packet
    uint64_t flowid;

    std::map<long,simtime_t> flow_completion_time;
    cOutVector FCT_Vector;
    cOutVector shortflow_FCT_Vector;
    cOutVector flowsize_Vector;
    cOutVector goodputVector;
    long BytesRcvd=0;
    long BytesSent=0;
    int num=0;
    int count=0;

    simtime_t sumFct=0;
//    simtime_t this_flow_creation_time=0;
    simtime_t last_pck_time = 0;
//    int this_flow_id=0;

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    // chooses random destination address
    virtual L3Address chooseDestAddr(int k);
    virtual void sendPacket(int packetlength);
    virtual void processPacket(Packet *msg);

    virtual void processStart();
    virtual void processSend();
    virtual void processStop();
    virtual void parseScript(const char *script);
    virtual void updateNextFlow(const char* TM);

    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

    virtual void setSocketOptions();
    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;
    virtual void socketClosed(UdpSocket *socket) override;
  public:
    HiUdpApp() {}
    ~HiUdpApp();
};

} // namespace inet

#endif // ifndef __INET_HIUdpAPP_H

