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

package inet.examples.crossDCN;

import ned.DatarateChannel;
import inet.HiNA.HiSwitch.HiSwitch;
import inet.HiNA.HiHost.HiHost;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;


network crossDCN
{
    @display("bgb=6968.4146,3052.7407;bgl=4");
    types:
        channel leafchannel extends DatarateChannel
        {
            delay = 10us;//10meters=50ns
            datarate = 10Gbps;
        }
        channel Spinechannel extends DatarateChannel
        {
            delay = 90us;
            datarate = 100Gbps;
        }
        channel crosschannel extends DatarateChannel
        {
            delay = 5ms;
            datarate = 100Mbps;
        }

    submodules:
        configurator: Ipv4NetworkConfigurator {
            @display("p=4585.59,864.7566;is=s");
            // + "<interface hosts='Spine1' address='192.168.5.x' netmask='255.0.0.0'/>"
            // + "<interface hosts='Spine2' address='202.197.5.x' netmask='255.255.0.0'/>"
            config = xml("<config>"
                        + "<interface hosts='Hosts1' address='192.168.1.x'  />"
                        + "<interface hosts='Hosts2' address='192.168.1.x'  />"
                        + "<interface hosts='Hosts3' address='192.168.1.x'  />"
                        + "<interface hosts='Hostb4' address='192.168.1.x'  />"
                        + "<interface hosts='Hostb5' address='192.168.1.x'  />"
                        + "<interface hosts='Hostb6' address='192.168.1.x'  />"
                        + "<interface hosts='Host7' address='192.168.1.x'   />"
                        + "<interface hosts='Host8' address='192.168.1.x'   />"
                        + "<interface hosts='Host9' address='192.168.1.x'   />"
                        + "<interface hosts='Host10' address='192.168.1.x'  />"
                        + "<interface hosts='leaf1' address='192.168.1.x' />"
                        + "<interface hosts='Hosta11' address='192.168.2.x'  />"
                        + "<interface hosts='Hosta12' address='192.168.2.x'  />"
                        + "<interface hosts='Hosta13' address='192.168.2.x'  />"
                        + "<interface hosts='Hostb14' address='192.168.2.x'  />"
                        + "<interface hosts='Hostb15' address='192.168.2.x'  />"
                        + "<interface hosts='Hostb16' address='192.168.2.x'  />"
                        + "<interface hosts='Host17' address='192.168.2.x'  />"
                        + "<interface hosts='Host18' address='192.168.2.x'  />"
                        + "<interface hosts='Host19' address='192.168.2.x'  />"
                        + "<interface hosts='Hostr20' address='192.168.2.x'  />"
                        + "<interface hosts='leaf2' address='192.168.2.x' />"
                        + "<interface hosts='Host21' address='202.197.3.x'  />"
                        + "<interface hosts='Host22' address='202.197.3.x'  />"
                        + "<interface hosts='Host23' address='202.197.3.x'  />"
                        + "<interface hosts='Host24' address='202.197.3.x'  />"
                        + "<interface hosts='Host25' address='202.197.3.x'  />"
                        + "<interface hosts='Host26' address='202.197.3.x'  />"
                        + "<interface hosts='Host27' address='202.197.3.x'  />"
                        + "<interface hosts='Host28' address='202.197.3.x'  />"
                        + "<interface hosts='Host29' address='202.197.3.x'  />"
                        + "<interface hosts='Hostr30' address='202.197.3.x'  />"
                        + "<interface hosts='leaf3' address='202.197.3.x' />"
                        + "<interface hosts='Host31' address='202.197.4.x'  />"
                        + "<interface hosts='Host32' address='202.197.4.x'  />"
                        + "<interface hosts='Host33' address='202.197.4.x'  />"
                        + "<interface hosts='Host34' address='202.197.4.x'  />"
                        + "<interface hosts='Host35' address='202.197.4.x'  />"
                        + "<interface hosts='Host36' address='202.197.4.x'  />"
                        + "<interface hosts='Host37' address='202.197.4.x'  />"
                        + "<interface hosts='Host38' address='202.197.4.x'  />"
                        + "<interface hosts='Host39' address='202.197.4.x'  />"
                        + "<interface hosts='Hostr40' address='202.197.4.x'  />"
                        + "<interface hosts='leaf4' address='202.197.4.x' />"
                        + "<interface among='Spine1 leaf1' address='192.168.1.x'  />"
                        + "<interface among='Spine1 leaf2' address='192.168.2.x'  />"
                        + "<interface among='Spine2 leaf3' address='202.197.3.x'  />"
                        + "<interface among='Spine2 leaf4' address='202.197.4.x'  />"
                       // + "<interface hosts='Spine1' address='192.168.5.x' netmask='255.0.0.0'/>"
                       // + "<interface hosts='Spine2' address='202.197.5.x' netmask='255.255.0.0'/>"
                        + "</config>");
        }
        Spine1: HiSwitch {
            @display("p=1400,1300");
            gates:
                ethg[3];
        }
        Spine2: HiSwitch {
            @display("p=3600,1300");
            gates:
                ethg[3];
        }
        leaf1: HiSwitch {
            @display("p=850,1700");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[11];
        }
        leaf2: HiSwitch {
            @display("p=1950,1700");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[11];
        }
        leaf3: HiSwitch {
            @display("p=3050,1700");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[11];
        }
        leaf4: HiSwitch {
            @display("p=4150,1700");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[11];
        }
        Hosts1: HiHost {
            @display("i=device/server;p=400,2000");
            gates:
                ethg[1];
        }
        Hosts2: HiHost {
            @display("i=device/server;p=500,2000");
            gates:
                ethg[1];
        }
        Hosts3: HiHost {
            @display("i=device/server;p=600,2000");
            gates:
                ethg[1];
        }
        Hostb4: HiHost {
            @display("i=device/server;p=700,2000");
            gates:
                ethg[1];
        }
        Hostb5: HiHost {
            @display("i=device/server;p=800,2000");
            gates:
                ethg[1];
        }
        Hostb6: HiHost {
            @display("i=device/server;p=900,2000");
            gates:
                ethg[1];
        }
        Host7: HiHost {
            @display("i=device/server;p=1000,2000");
            gates:
                ethg[1];
        }
        Host8: HiHost {
            @display("i=device/server;p=1100,2000");
            gates:
                ethg[1];
        }
        Host9: HiHost {
            @display("i=device/server;p=1200,2000");
            gates:
                ethg[1];
        }
        Host10: HiHost {
            @display("i=device/server;p=1300,2000");
            gates:
                ethg[1];
        }
        Hosta11: HiHost {
            @display("i=device/server;p=1500,2000");
            gates:
                ethg[1];
        }
        Hosta12: HiHost {
            @display("i=device/server;p=1600,2000");
            gates:
                ethg[1];
        }
        Hosta13: HiHost {
            @display("i=device/server;p=1700,2000");
            gates:
                ethg[1];
        }
        Hostb14: HiHost {
            @display("i=device/server;p=1800,2000");
            gates:
                ethg[1];
        }
        Hostb15: HiHost {
            @display("i=device/server;p=1900,2000");
            gates:
                ethg[1];
        }
        Hostb16: HiHost {
            @display("i=device/server;p=2000,2000");
            gates:
                ethg[1];
        }
        Host17: HiHost {
            @display("i=device/server;p=2100,2000");
            gates:
                ethg[1];
        }
        Host18: HiHost {
            @display("i=device/server;p=2200,2000");
            gates:
                ethg[1];
        }
        Host19: HiHost {
            @display("i=device/server;p=2300,2000");
            gates:
                ethg[1];
        }
        Hostr20: HiHost {
            @display("i=device/server;p=2400,2000");
            gates:
                ethg[1];
        }
        Host21: HiHost {
            @display("i=device/server;p=2600,2000");
            gates:
                ethg[1];
        }
        Host22: HiHost {
            @display("i=device/server;p=2700,2000");
            gates:
                ethg[1];
        }
        Host23: HiHost {
            @display("i=device/server;p=2800,2000");
            gates:
                ethg[1];
        }
        Host24: HiHost {
            @display("i=device/server;p=2900,2000");
            gates:
                ethg[1];
        }
        Host25: HiHost {
            @display("i=device/server;p=3000,2000");
            gates:
                ethg[1];
        }
        Host26: HiHost {
            @display("i=device/server;p=3100,2000");
            gates:
                ethg[1];
        }
        Host27: HiHost {
            @display("i=device/server;p=3200,2000");
            gates:
                ethg[1];
        }
        Host28: HiHost {
            @display("i=device/server;p=3300,2000");
            gates:
                ethg[1];
        }
        Host29: HiHost {
            @display("i=device/server;p=3400,2000");
            gates:
                ethg[1];
        }
        Hostr30: HiHost {
            @display("i=device/server;p=3500,2000");
            gates:
                ethg[1];
        }
        Host31: HiHost {
            @display("i=device/server;p=3700,2000");
            gates:
                ethg[1];
        }
        Host32: HiHost {
            @display("i=device/server;p=3800,2000");
            gates:
                ethg[1];
        }
        Host33: HiHost {
            @display("i=device/server;p=3900,2000");
            gates:
                ethg[1];
        }
        Host34: HiHost {
            @display("i=device/server;p=4000,2000");
            gates:
                ethg[1];
        }
        Host35: HiHost {
            @display("i=device/server;p=4100,2000");
            gates:
                ethg[1];
        }
        Host36: HiHost {
            @display("i=device/server;p=4200,2000");
            gates:
                ethg[1];
        }
        Host37: HiHost {
            @display("i=device/server;p=4300,2000");
            gates:
                ethg[1];
        }
        Host38: HiHost {
            @display("i=device/server;p=4400,2000");
            gates:
                ethg[1];
        }
        Host39: HiHost {
            @display("i=device/server;p=4500,2000");
            gates:
                ethg[1];
        }
        Hostr40: HiHost {
            @display("i=device/server;p=4600,2000");
            gates:
                ethg[1];
        }
    connections:
        Hosts1.ethg++ <--> leafchannel <--> leaf1.ethg++;
        Hosts2.ethg++ <--> leafchannel <--> leaf1.ethg++;
        Hosts3.ethg++ <--> leafchannel <--> leaf1.ethg++;
        Hostb4.ethg++ <--> leafchannel <--> leaf1.ethg++;
        Hostb5.ethg++ <--> leafchannel <--> leaf1.ethg++;
        Hostb6.ethg++ <--> leafchannel <--> leaf1.ethg++;
        Host7.ethg++ <--> leafchannel <--> leaf1.ethg++;
        Host8.ethg++ <--> leafchannel <--> leaf1.ethg++;
        Host9.ethg++ <--> leafchannel <--> leaf1.ethg++;
        Host10.ethg++ <--> leafchannel <--> leaf1.ethg++;

        Hosta11.ethg++ <--> leafchannel <--> leaf2.ethg++;
        Hosta12.ethg++ <--> leafchannel <--> leaf2.ethg++;
        Hosta13.ethg++ <--> leafchannel <--> leaf2.ethg++;
        Hostb14.ethg++ <--> leafchannel <--> leaf2.ethg++;
        Hostb15.ethg++ <--> leafchannel <--> leaf2.ethg++;
        Hostb16.ethg++ <--> leafchannel <--> leaf2.ethg++;
        Host17.ethg++ <--> leafchannel <--> leaf2.ethg++;
        Host18.ethg++ <--> leafchannel <--> leaf2.ethg++;
        Host19.ethg++ <--> leafchannel <--> leaf2.ethg++;
        Hostr20.ethg++ <--> leafchannel <--> leaf2.ethg++;

        Host21.ethg++ <--> leafchannel <--> leaf3.ethg++;
        Host22.ethg++ <--> leafchannel <--> leaf3.ethg++;
        Host23.ethg++ <--> leafchannel <--> leaf3.ethg++;
        Host24.ethg++ <--> leafchannel <--> leaf3.ethg++;
        Host25.ethg++ <--> leafchannel <--> leaf3.ethg++;
        Host26.ethg++ <--> leafchannel <--> leaf3.ethg++;
        Host27.ethg++ <--> leafchannel <--> leaf3.ethg++;
        Host28.ethg++ <--> leafchannel <--> leaf3.ethg++;
        Host29.ethg++ <--> leafchannel <--> leaf3.ethg++;
        Hostr30.ethg++ <--> leafchannel <--> leaf3.ethg++;

        Host31.ethg++ <--> leafchannel <--> leaf4.ethg++;
        Host32.ethg++ <--> leafchannel <--> leaf4.ethg++;
        Host33.ethg++ <--> leafchannel <--> leaf4.ethg++;
        Host34.ethg++ <--> leafchannel <--> leaf4.ethg++;
        Host35.ethg++ <--> leafchannel <--> leaf4.ethg++;
        Host36.ethg++ <--> leafchannel <--> leaf4.ethg++;
        Host37.ethg++ <--> leafchannel <--> leaf4.ethg++;
        Host38.ethg++ <--> leafchannel <--> leaf4.ethg++;
        Host39.ethg++ <--> leafchannel <--> leaf4.ethg++;
        Hostr40.ethg++ <--> leafchannel <--> leaf4.ethg++;

        Spine1.ethg++ <--> crosschannel <--> Spine2.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf1.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf2.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf3.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf4.ethg++;
}
