#include <map>
#include <queue>
#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../applications/base/ApplicationPacket_m.h"

#include "../../common/TagBase_m.h"
#include "../../common/TimeTag_m.h"
#include "../../common/INETDefs.h"
#include "../../common/INETUtils.h"
#include "../../common/IProtocolRegistrationListener.h"
#include "../../common/LayeredProtocolBase.h"
#include "../../common/lifecycle/ILifecycle.h"
#include "../../common/lifecycle/ModuleOperations.h"
#include "../../common/lifecycle/NodeStatus.h"
#include "../../common/ModuleAccess.h"
#include "../../common/packet/Message.h"
#include "../../common/ProtocolTag_m.h"
#include "../../common/checksum/TcpIpChecksum.h"
#include "../../common/TagBase_m.h"
#include "../../common/IProtocolRegistrationListener.h"
#include "../../common/socket/SocketTag_m.h"

#include "../../networklayer/arp/ipv4/ArpPacket_m.h"
#include "../../networklayer/common/DscpTag_m.h"
#include "../../networklayer/common/EcnTag_m.h"
#include "../../networklayer/common/FragmentationTag_m.h"
#include "../../networklayer/common/HopLimitTag_m.h"
#include "../../networklayer/common/L3AddressTag_m.h"
#include "../../networklayer/common/L3Tools.h"
#include "../../networklayer/common/MulticastTag_m.h"
#include "../../networklayer/common/NextHopAddressTag_m.h"
#include "../../networklayer/contract/IArp.h"
#include "../../networklayer/contract/IInterfaceTable.h"
#include "../../networklayer/contract/ipv4/Ipv4SocketCommand_m.h"
#include "../../networklayer/ipv4/IcmpHeader_m.h"
#include "../../networklayer/ipv4/IIpv4RoutingTable.h"
#include "../../networklayer/ipv4/Ipv4.h"
#include "../../networklayer/ipv4/Ipv4Header_m.h"
#include "../../networklayer/ipv4/Ipv4InterfaceData.h"
#include "../../networklayer/contract/IArp.h"
#include "../../networklayer/ipv4/Icmp.h"
#include "../../networklayer/contract/INetfilter.h"
#include "../../networklayer/contract/INetworkProtocol.h"
#include "../../networklayer/ipv4/Ipv4FragBuf.h"
#include "../../networklayer/common/L3Address.h"

#include "../../linklayer/common/InterfaceTag_m.h"
#include "../../linklayer/common/MacAddressTag_m.h"

#include "../../transportlayer/common/L4PortTag_m.h"
#include "../../transportlayer/common/L4Tools.h"
#include "../../transportlayer/udp/UdpHeader_m.h"
#include "../../transportlayer/udp/Udp.h"

#include "../../HiNA/Messages/TimerMsg/TimerMsg_m.h"
#include "../../HiNA/Messages/HiTag/HiTag_m.h"
