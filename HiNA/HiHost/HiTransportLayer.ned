//
// Copyright (C) OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

package inet.HiNA.HiHost;

import inet.HiNA.CC.UdpCC;
import inet.common.MessageDispatcher;
import inet.transportlayer.contract.ISctp;
import inet.transportlayer.contract.ITcp;
import inet.transportlayer.contract.IUdp;

module HiTransportLayer extends HiNetworkLayer
{
    parameters:
        bool hasUdp = default(firstAvailableOrEmpty("Udp") != "");
        bool hasTcp = default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc") != "");
        bool hasSctp = default(false);
        @figure[transportLayer](type=rectangle; pos=1070,161; size=200,20; fillColor=#ff0000; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[transportLayer.title](type=text; pos=1245,161; anchor=ne; text="transport layer");
    submodules:
        udp: <default(firstAvailableOrEmpty("Udp"))> like IUdp if hasUdp {
            parameters:
                @display("p=595.38464,173.07693;is=s");
        }
        tcp: <default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc"))> like ITcp if hasTcp {
            parameters:
                @display("p=829.464,172.056;is=s");
        }
        sctp: <default(firstAvailableOrEmpty("Sctp"))> like ISctp if hasSctp {
            parameters:
                @display("p=1061.5499,222.075;is=s");
        }
        tn: MessageDispatcher {
            parameters:
                @display("p=750,296;b=1000,5,,,,1");
        }
        UdpCC: <default("DCQCN")> like UdpCC {
            @display("p=595.38464,251.53847;is=s");
        }
        //TcpCC: <default("TcpFlowShaping")> like HiCC {
          //  @display("p=829.464,249.096;is=s");
        //}
    connections allowunconnected:

        //udp.ipOut --> tn.in++ if hasUdp;
       // udp.ipIn <-- tn.out++ if hasUdp;

        UdpCC.lowerOut --> tn.in++ if hasUdp;
        UdpCC.lowerIn <-- tn.out++ if hasUdp;
        UdpCC.upperOut --> udp.ipIn if hasUdp;
        UdpCC.upperIn <-- udp.ipOut if hasUdp;

        //TcpCC.lowerOut --> tn.in++ if hasTcp;
        //TcpCC.lowerIn <-- tn.out++ if hasTcp;
        //TcpCC.upperOut --> tcp.ipIn if hasTcp;
        //TcpCC.upperIn <-- tcp.ipOut if hasTcp;
        
        tcp.ipOut --> tn.in++ if hasTcp;
        tcp.ipIn <-- tn.out++ if hasTcp;

        sctp.ipOut --> tn.in++ if hasSctp;
        tn.out++ --> sctp.ipIn if hasSctp;

        tn.out++ --> ipv4.transportIn if hasIpv4;
        tn.in++ <-- ipv4.transportOut if hasIpv4;

        tn.out++ --> ipv6.transportIn if hasIpv6;
        tn.in++ <-- ipv6.transportOut if hasIpv6;

        tn.out++ --> generic.transportIn if hasGn;
        tn.in++ <-- generic.transportOut if hasGn;

        tn.out++ --> nl.in++;
        tn.in++ <-- nl.out++;
}
