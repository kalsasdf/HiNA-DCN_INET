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

package inet.examples.HiNA.test.parkinglot;

import inet.HiNA.HiSwitch.HiSwitch;
import inet.HiNA.HiHost.HiHost;
import ned.DatarateChannel;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;

network testparkinglot
{
    types:
        channel Hostchannel extends DatarateChannel
        {
            delay = 50ns;
            datarate = 10Gbps;
        }
    submodules:
        configurator: Ipv4NetworkConfigurator;
        Host1: HiHost {
            @display("p=50,180");
        }
        Host2: HiHost {
            @display("p=150,256");
        }
        Host3: HiHost {
            @display("p=250,256");
        }
        Host4: HiHost {
            @display("p=350,256");
        }
        Host5: HiHost {
            @display("p=450,256");
        }
        Host6: HiHost {
            @display("p=550,256");
        }
        Host7: HiHost {
            @display("p=650,180");
        }
        Router1: HiSwitch {
            @display("p=150,180");
        }
        Router2: HiSwitch {
            @display("p=250,180");
        }
        Router3: HiSwitch {
            @display("p=350,180");
        }
        Router4: HiSwitch {
            @display("p=450,180");
        }
        Router5: HiSwitch {
            @display("p=550,180");
        }
    connections:
        Host1.ethg++ <--> Hostchannel <--> Router1.ethg++;
        Host2.ethg++ <--> Hostchannel <--> Router1.ethg++;
        Router1.ethg++ <--> Hostchannel <--> Router2.ethg++;
        Host3.ethg++ <--> Hostchannel <--> Router2.ethg++;
        Router2.ethg++ <--> Hostchannel <--> Router3.ethg++;
        Host4.ethg++ <--> Hostchannel <--> Router3.ethg++;
        Router3.ethg++ <--> Hostchannel <--> Router4.ethg++;
        Host5.ethg++ <--> Hostchannel <--> Router4.ethg++;
        Router4.ethg++ <--> Hostchannel <--> Router5.ethg++;
        Host6.ethg++ <--> Hostchannel <--> Router5.ethg++;
        Router5.ethg++ <--> Hostchannel <--> Host7.ethg++;
}
