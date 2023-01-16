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

#include "inet/common/INETDefs.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/applications/tcpapp/TcpAppBase.h"
#include "inet/HiNA/Messages/HiTag/HiTag_m.h"
#include "inet/common/socket/SocketMap.h"


namespace inet {


/**
 * Single-connection TCP application.
 */
class INET_API TcpApp : public TcpAppBase
{
  protected:
    // parameters
    struct Command {
        simtime_t tSend;
        long numBytes = 0;
        Command(simtime_t t, long n) { tSend = t; numBytes = n; }
    };
    typedef std::vector<Command> CommandVector;
    CommandVector commands;
    int commandIndex = -1;

    simtime_t tOpen;
    simtime_t tSend;
    simtime_t tClose;
    int sendBytes = 0;
    const char *packetName = nullptr;

    // state
    cMessage *timeoutMsg = nullptr;

    // statistics
    int numSent = 0;
    int numReceived = 0;
    static uint32_t flowid;

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
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    virtual void parseScript(const char *script);
    virtual Packet *createDataPacket(long sendBytes);
    virtual void sendData();

    virtual void handleTimer(cMessage *msg) override;
    virtual void socketEstablished(TcpSocket *socket) override;
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

