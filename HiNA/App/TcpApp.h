//
// Copyright (C) 2004 Andras Varga
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
#ifndef __INET_TCPFLOWAPP_H
#define __INET_TCPFLOWAPP_H

#include <vector>
#include <map>
#include <fstream>

#include "GlobalFlowId.h"
#include "inet/HiNA/Messages/HiTag/HiTag_m.h"

#include "inet/common/INETDefs.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/common/socket/SocketMap.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"

#include "inet/applications/tcpapp/TcpAppBase.h"
#include "inet/applications/base/ApplicationPacket_m.h"

#include "inet/networklayer/common/L3AddressResolver.h"


namespace inet {


/**
 * Single-connection TCP application.
 */
class INET_API TcpApp : public TcpAppBase
{
  protected:
    struct Command {
        simtime_t tSend;
        long numBytes = 0;
        Command(simtime_t t, long n) { tSend = t; numBytes = n; }
    };
    typedef std::vector<Command> CommandVector;
    CommandVector commands;
    int commandIndex = -1;
    std::vector<L3Address> connectAddresses;
    std::vector<std::string> destAddressStr;

    // parameters
    simtime_t tOpen=0;
    simtime_t tSend=0;
    simtime_t tClose=0;
    const char *packetName = nullptr;
    const char *trafficMode = nullptr;
    uint longflow=0;
    double packetLength=0;
    int AppPriority;
    double linkSpeed;
    bool Enablepoisson;
    double workLoad;

    // state
    cMessage *selfMsg = nullptr;
    SocketMap socketMap;

    // statistics
    int numSent = 0;
    int numReceived = 0;
    std::map<long,simtime_t> flow_completion_time;
    std::map<uint64_t,long> flowsize_Map;
    cOutVector FCT_Vector;
    cOutVector shortflow_FCT_Vector;
    cOutVector flowsize_Vector;
    cOutVector goodputVector;
    simtime_t sumFct=0;
    long BytesRcvd=0;
    long BytesSent=0;
    simtime_t last_pck_time=0;


  protected:

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void initialize(int stage) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    virtual void connect() override;
    virtual L3Address chooseDestAddr(int k);
    virtual void parseScript(const char *script);
    virtual void updateNextFlow(const char* TM);
    virtual Packet *createDataPacket(long sendBytes);
    virtual void sendData();

    virtual void handleTimer(cMessage *msg) override;
    virtual void socketEstablished(TcpSocket *socket) override;
    virtual void socketAvailable(TcpSocket *socket, TcpAvailableInfo *availableInfo) override;
    virtual void socketDataArrived(TcpSocket *socket, Packet *pck, bool urgent) override;
    virtual void socketClosed(TcpSocket *socket) override;
    virtual void socketFailure(TcpSocket *socket, int code) override;

    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

  public:
    TcpApp() {}
    virtual ~TcpApp();
};

} // namespace inet

#endif // ifndef __INET_TCPFLOWAPP_H

