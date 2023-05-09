//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package inet.examples.HiNA.test.dumbbell;

import inet.HiNA.HiSwitch.HiSwitch;
import inet.HiNA.HiHost.HiHost;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import ned.DatarateChannel;
import inet.common.misc.ThruputMeteringChannel;
import inet.node.inet.StandardHost;


//
// TODO auto-generated type
//
network testdumbbell
{
    @display("bgb=1119.785,595.72");
    types:
        channel agchannel extends DatarateChannel
        {
            delay = 50ns;//1meter=5ns
            datarate = 10Gbps;
        }
    submodules:
        Hostr1: HiHost {
            @display("p=866.885,25.29;i=device/server");
        }
        Hosts1: HiHost {
            @display("p=325.96,25.29;i=device/server");
        }
        Router1: HiSwitch {
            @display("p=536.71,282.405");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=68,37");
        }
        Hosts2: HiHost {
            @display("p=325.96,81.49;i=device/server");
        }
        Hosts3: HiHost {
            @display("p=325.96,139.095;i=device/server");
        }
        Hosts4: HiHost {
            @display("p=325.96,202.32;i=device/server");
        }
        Hosts5: HiHost {
            @display("p=325.96,258.52;i=device/server");
        }
        Router2: HiSwitch {
            @display("p=685.64,282.405");
        }
        Hostr2: HiHost {
            @display("p=866.885,81.49;i=device/server");
        }
        Hostr3: HiHost {
            @display("p=866.885,139.095;i=device/server");
        }
        Hostr4: HiHost {
            @display("p=866.885,202.32;i=device/server");
        }
        Hostr5: HiHost {
            @display("p=866.885,257.115;i=device/server");
        }
        Hosts6: HiHost {
            @display("p=325.96,314.72;i=device/server");
        }
        Hosts7: HiHost {
            @display("p=325.96,370.92;i=device/server");
        }
        Hosts8: HiHost {
            @display("p=325.96,428.525;i=device/server");
        }
        Hosts9: HiHost {
            @display("p=325.96,491.75;i=device/server");
        }
        Hosts10: HiHost {
            @display("p=325.96,546.545;i=device/server");
        }
        Hostr6: HiHost {
            @display("p=866.885,314.72;i=device/server");
        }
        Hostr7: HiHost {
            @display("p=866.885,370.92;i=device/server");
        }
        Hostr8: HiHost {
            @display("p=866.885,428.525;i=device/server");
        }
        Hostr9: HiHost {
            @display("p=866.885,491.75;i=device/server");
        }
        Hostr10: HiHost {
            @display("p=866.885,546.545;i=device/server");
        }
    connections:
        Hosts1.ethg++ <--> agchannel <--> Router1.ethg++;
        Hosts2.ethg++ <--> agchannel <--> Router1.ethg++;
        Hosts3.ethg++ <--> agchannel <--> Router1.ethg++;
        Hosts4.ethg++ <--> agchannel <--> Router1.ethg++;
        Hosts5.ethg++ <--> agchannel <--> Router1.ethg++;
        Hosts6.ethg++ <--> agchannel <--> Router1.ethg++;
        Hosts7.ethg++ <--> agchannel <--> Router1.ethg++;
        Hosts8.ethg++ <--> agchannel <--> Router1.ethg++;
        Hosts9.ethg++ <--> agchannel <--> Router1.ethg++;
        Hosts10.ethg++ <--> agchannel <--> Router1.ethg++;
        Router2.ethg++ <--> agchannel <--> Hostr1.ethg++;
        Router2.ethg++ <--> agchannel <--> Hostr2.ethg++;
        Router2.ethg++ <--> agchannel <--> Hostr3.ethg++;
        Router2.ethg++ <--> agchannel <--> Hostr4.ethg++;
        Router2.ethg++ <--> agchannel <--> Hostr5.ethg++;
        Router2.ethg++ <--> agchannel <--> Hostr6.ethg++;
        Router2.ethg++ <--> agchannel <--> Hostr7.ethg++;
        Router2.ethg++ <--> agchannel <--> Hostr8.ethg++;
        Router2.ethg++ <--> agchannel <--> Hostr9.ethg++;
        Router2.ethg++ <--> agchannel <--> Hostr10.ethg++;
        Router1.ethg++ <--> agchannel <--> Router2.ethg++;
}