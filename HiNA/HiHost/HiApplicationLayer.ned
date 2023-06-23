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

import inet.applications.contract.IApp;
import inet.common.MessageDispatcher;
import inet.HiNA.App.GlobalFlowId;

module HiApplicationLayer extends HiTransportLayer
{
    parameters:
        int numApps = default(0);
        @figure[applicationLayer](type=rectangle; pos=1070,11; size=200,20; lineColor=#808080; cornerRadius=5; fillColor=#ffff00; fillOpacity=0.1);
        @figure[applicationLayer.title](type=text; pos=1245,11; anchor=ne; text="application layer");

        @display("bgb=1255.2749,771.75");
    submodules:
        app[numApps]: <> like IApp {
            parameters:
                @display("p=750.6788,64.52375,row,150;is=s");
        }
        at: MessageDispatcher {
            parameters:
                @display("p=749.10504,141.63751;b=1000,5,,,,1");
        }
        GlobalFlowId: GlobalFlowId {
            @display("p=893.89,64.52375;is=s");
        }
    connections allowunconnected:
        for i=0..numApps-1 {
            app[i].socketOut --> at.in++;
            app[i].socketIn <-- at.out++;
        }

        at.out++ --> udp.appIn if hasUdp;
        at.in++ <-- udp.appOut if hasUdp;

        at.out++ --> tcp.appIn if hasTcp;
        at.in++ <-- tcp.appOut if hasTcp;

        at.out++ --> sctp.appIn if hasSctp;
        at.in++ <-- sctp.appOut if hasSctp;

        at.out++ --> tn.in++;
        at.in++ <-- tn.out++;
}
