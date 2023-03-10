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

package inet.examples.HiNA.test;


import inet.common.misc.ThruputMeter;
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
            delay = 2us;
            datarate = 10Gbps;
        }
    submodules:
        r1: HiHost {
            @display("p=866.885,25.29;i=device/server");
        }
        s1: HiHost {
            @display("p=325.96,25.29;i=device/server");
        }
        Router1: HiSwitch {
            @display("p=536.71,282.405");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=68,37");
        }
        s2: HiHost {
            @display("p=325.96,81.49;i=device/server");
        }
        s3: HiHost {
            @display("p=325.96,139.095;i=device/server");
        }
        s4: HiHost {
            @display("p=325.96,202.32;i=device/server");
        }
        s5: HiHost {
            @display("p=325.96,258.52;i=device/server");
        }
        Router2: HiSwitch {
            @display("p=685.64,282.405");
        }
        r2: HiHost {
            @display("p=866.885,81.49;i=device/server");
        }
        r3: HiHost {
            @display("p=866.885,139.095;i=device/server");
        }
        r4: HiHost {
            @display("p=866.885,202.32;i=device/server");
        }
        r5: HiHost {
            @display("p=866.885,257.115;i=device/server");
        }
        s6: HiHost {
            @display("p=325.96,314.72;i=device/server");
        }
        s7: HiHost {
            @display("p=325.96,370.92;i=device/server");
        }
        s8: HiHost {
            @display("p=325.96,428.525;i=device/server");
        }
        s9: HiHost {
            @display("p=325.96,491.75;i=device/server");
        }
        s10: HiHost {
            @display("p=325.96,546.545;i=device/server");
        }
        r6: HiHost {
            @display("p=866.885,314.72;i=device/server");
        }
        r7: HiHost {
            @display("p=866.885,370.92;i=device/server");
        }
        r8: HiHost {
            @display("p=866.885,428.525;i=device/server");
        }
        r9: HiHost {
            @display("p=866.885,491.75;i=device/server");
        }
        r10: HiHost {
            @display("p=866.885,546.545;i=device/server");
        }
    connections:
        s1.ethg++ <--> agchannel <--> Router1.ethg++;
        s2.ethg++ <--> agchannel <--> Router1.ethg++;
        s3.ethg++ <--> agchannel <--> Router1.ethg++;
        s4.ethg++ <--> agchannel <--> Router1.ethg++;
        s5.ethg++ <--> agchannel <--> Router1.ethg++;
        s6.ethg++ <--> agchannel <--> Router1.ethg++;
        s7.ethg++ <--> agchannel <--> Router1.ethg++;
        s8.ethg++ <--> agchannel <--> Router1.ethg++;
        s9.ethg++ <--> agchannel <--> Router1.ethg++;
        s10.ethg++ <--> agchannel <--> Router1.ethg++;
        Router2.ethg++ <--> agchannel <--> r1.ethg++;
        Router2.ethg++ <--> agchannel <--> r2.ethg++;
        Router2.ethg++ <--> agchannel <--> r3.ethg++;
        Router2.ethg++ <--> agchannel <--> r4.ethg++;
        Router2.ethg++ <--> agchannel <--> r5.ethg++;
        Router2.ethg++ <--> agchannel <--> r6.ethg++;
        Router2.ethg++ <--> agchannel <--> r7.ethg++;
        Router2.ethg++ <--> agchannel <--> r8.ethg++;
        Router2.ethg++ <--> agchannel <--> r9.ethg++;
        Router2.ethg++ <--> agchannel <--> r10.ethg++;
        Router1.ethg++ <--> agchannel <--> Router2.ethg++;
}