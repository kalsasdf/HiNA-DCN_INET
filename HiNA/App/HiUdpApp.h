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

#include "inet/HiNA/Messages/HiTag/HiTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/common/clock/ClockUserModuleMixin.h"


namespace inet {

/**
 * UDP application. See NED for more info.
 */
class INET_API HiUdpApp : public ClockUserModuleMixin<ApplicationBase>, public UdpSocket::ICallback
{
  protected:
    enum SelfMsgKinds { START = 1, SEND, STOP };

    // parameters
    std::vector<L3Address> connectAddresses;
    std::vector<std::string> destAddressStr;
    int localPort = -1, connectPort = -1;
    simtime_t startTime;
    simtime_t stopTime;
    const char *packetName = nullptr;
    double packetLength;
    uint32_t AppPriority;
    const char *trafficMode = nullptr;
    double linkSpeed;
    double workLoad;

    // state
    UdpSocket socket;
    cMessage *selfMsg = nullptr;

    // statistics
    int numSent = 0;
    int numReceived = 0;
    static uint32_t flowid;    // counter for generating a global number for each packet

    std::map<long,simtime_t> flow_completion_time;
    cOutVector FCT_Vector;
    cOutVector goodputVector;
    long BytesRcvd=0;
    long BytesSent=0;

    simtime_t sumFct=0;
    simtime_t this_flow_creation_time=0;
    simtime_t last_flow_creation_time=0;
    simtime_t last_pck_time=0;
    int this_flow_id=0;
    int last_flow_id=0;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    // chooses random destination address
    virtual L3Address chooseDestAddr(int k);
    virtual void sendPacket();
    virtual void processPacket(Packet *msg);

    virtual void processStart();
    virtual void processSend();
    virtual void processStop();
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

