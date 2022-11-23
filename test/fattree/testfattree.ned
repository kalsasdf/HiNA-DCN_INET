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

package inet.examples.HiNA.test.fattree;

import ned.DatarateChannel;
import inet.HiNA.HiSwitch.HiSwitch;
import inet.HiNA.HiHost.HiHost;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;

network testfattree
{
    @display("bgb=5256,2833;bgl=4");
    types:
        channel Hostchannel extends DatarateChannel
        {
            delay = 5us;//10meters=50us
            datarate = 1Gbps;
        }
        channel Switchchannel extends DatarateChannel
        {
            delay = 10us;
            datarate = 10Gbps;
        }

    submodules:
        configurator: Ipv4NetworkConfigurator {
            @display("p=5534.8325,176.2981;is=s");
            config = xml("<config>"
                       // + "<interface host='Host1' name='lo0' address='0.0.0.0'  />"
                        + "<interface hosts='Host1' address='10.0.0.x' />"
                       // + "<interface host='Host2' name='lo0' address='10.0.0.2'  />"
                        + "<interface hosts='Host2' address='10.0.0.x' />"
                        + "<interface hosts='Host3' address='10.0.1.x' />"
                        + "<interface hosts='Host4' address='10.0.1.x' />"
                        + "<interface hosts='Host5' address='10.1.0.x' />"
                        + "<interface hosts='Host6' address='10.1.0.x' />"
                        + "<interface hosts='Host7' address='10.1.1.x' />"
                        + "<interface hosts='Host8' address='10.1.1.x' />"
                        + "<interface hosts='Host9' address='10.2.0.x' />"
                        + "<interface hosts='Host10' address='10.2.0.x' />"
                        + "<interface hosts='Host11' address='10.2.1.x' />"
                        + "<interface hosts='Host12' address='10.2.1.x' />"
                        + "<interface hosts='Host13' address='10.3.0.x' />"
                        + "<interface hosts='Host14' address='10.3.0.x' />"
                        + "<interface hosts='Host15' address='10.3.1.x' />"
                        + "<interface hosts='Host16' address='10.3.1.x' />"
                        + "<interface hosts='Core1' address='10.4.1.x' netmask='255.255.255.252' />"
                        + "<interface hosts='Core2' address='10.4.1.x' netmask='255.255.255.252' />"
                        + "<interface hosts='Core3' address='10.4.2.x' netmask='255.255.255.252' />"
                        + "<interface hosts='Core4' address='10.4.2.x' netmask='255.255.255.252' />"
                        //+ "<interface among='Agre1 Core*' address='10.0.2.x' />"
                        //+ "<interface among='Agre1 Edge1' address='10.0.0.x' />"
                        //+ "<interface among='Agre1 Edge2' address='10.0.1.x' />"
                        //+ "<interface among='Agre2 Core*' address='10.0.3.x' />"
                        //+ "<interface among='Agre2 Edge1' address='10.0.0.x' />"
                        //+ "<interface among='Agre2 Edge2' address='10.0.1.x' />"
                        //+ "<interface among='Agre3 Core*' address='10.1.2.x' />"
                        //+ "<interface among='Agre3 Edge3' address='10.1.0.x' />"
                        //+ "<interface among='Agre3 Edge4' address='10.1.1.x' />"
                        //+ "<interface among='Agre4 Core*' address='10.1.3.x' />"
                        //+ "<interface among='Agre4 Edge3' address='10.1.0.x' />"
                        //+ "<interface among='Agre4 Edge4' address='10.1.1.x' />"
                        //+ "<interface among='Agre5 Core*' address='10.2.2.x' />"
                        //+ "<interface among='Agre5 Edge5' address='10.2.0.x' />"
                        //+ "<interface among='Agre5 Edge6' address='10.2.1.x' />"
                        //+ "<interface among='Agre6 Core*' address='10.2.3.x' />"
                        //+ "<interface among='Agre6 Edge5' address='10.2.0.x' />"
                        //+ "<interface among='Agre6 Edge6' address='10.2.1.x' />"
                        //+ "<interface among='Agre7 Core*' address='10.3.2.x' />"
                        //+ "<interface among='Agre7 Edge7' address='10.3.0.x' />"
                        //+ "<interface among='Agre7 Edge8' address='10.3.1.x' />"
                        //+ "<interface among='Agre8 Core*' address='10.3.3.x' />"
                        //+ "<interface among='Agre8 Edge7' address='10.3.0.x' />"
                        //+ "<interface among='Agre8 Edge8' address='10.3.1.x' />"
                        + "<interface hosts='Agre1' address='10.0.2.x' netmask='255.255.255.252' />"
                        + "<interface hosts='Agre2' address='10.0.3.x' netmask='255.255.255.252' />"
                        + "<interface hosts='Agre3' address='10.1.2.x' netmask='255.255.255.252' />"
                        + "<interface hosts='Agre4' address='10.1.3.x' netmask='255.255.255.252' />"
                        + "<interface hosts='Agre5' address='10.2.2.x' netmask='255.255.255.252' />"
                        + "<interface hosts='Agre6' address='10.2.3.x' netmask='255.255.255.252' />"
                        + "<interface hosts='Agre7' address='10.3.2.x' netmask='255.255.255.252' />"
                        + "<interface hosts='Agre8' address='10.3.3.x' netmask='255.255.255.252' />"
                        + "<interface hosts='Edge1' address='10.0.0.x' />"
                        + "<interface hosts='Edge2' address='10.0.1.x' />"
                        + "<interface hosts='Edge3' address='10.1.0.x' />"
                        + "<interface hosts='Edge4' address='10.1.1.x' />"
                        + "<interface hosts='Edge5' address='10.2.0.x' />"
                        + "<interface hosts='Edge6' address='10.2.1.x' />"
                        + "<interface hosts='Edge7' address='10.3.0.x' />"
                        + "<interface hosts='Edge8' address='10.3.1.x' />"
                        //+ "<autoroute metric='delay'>"
                        //+ "<link interfaces='Edge1.eth2 Agre1.eth0' cost='2'/>"
                        //+ "</autoroute>"
                        + "</config>");
        }
        Core1: HiSwitch {
            @display("p=1000,1300");
            gates:
                ethg[4];
        }
        Core2: HiSwitch {
            @display("p=2000,1300");
            gates:
                ethg[4];
        }
        Core3: HiSwitch {
            @display("p=3000,1300");
            gates:
                ethg[4];
        }
        Core4: HiSwitch {
            @display("p=4000,1300");
            gates:
                ethg[4];
        }
        Agre1: HiSwitch {
            @display("p=800,1700");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Agre2: HiSwitch {
            @display("p=1200,1700");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Agre3: HiSwitch {
            @display("p=1800,1700");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Agre4: HiSwitch {
            @display("p=2200,1700");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Agre5: HiSwitch {
            @display("p=2800,1700");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Agre6: HiSwitch {
            @display("p=3200,1700");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Agre7: HiSwitch {
            @display("p=3800,1700");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Agre8: HiSwitch {
            @display("p=4200,1700");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Edge1: HiSwitch {
            @display("p=800,1900");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Edge2: HiSwitch {
            @display("p=1200,1900");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Edge3: HiSwitch {
            @display("p=1800,1900");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Edge4: HiSwitch {
            @display("p=2200,1900");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Edge5: HiSwitch {
            @display("p=2800,1900");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Edge6: HiSwitch {
            @display("p=3200,1900");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Edge7: HiSwitch {
            @display("p=3800,1900");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Edge8: HiSwitch {
            @display("p=4200,1900");
            //eth[0..9].bitrate=10Gbps;
            //eth[10].bitrate=50Gbps;
            gates:
                ethg[4];
        }
        Host1: HiHost {
            @display("i=device/server;p=700,2200");
            gates:
                ethg[1];
        }
        Host2: HiHost {
            @display("i=device/server;p=900,2200");
            gates:
                ethg[1];
        }
        Host3: HiHost {
            @display("i=device/server;p=1100,2200");
            gates:
                ethg[1];
        }
        Host4: HiHost {
            @display("i=device/server;p=1300,2200");
            gates:
                ethg[1];
        }
        Host5: HiHost {
            @display("i=device/server;p=1700,2200");
            gates:
                ethg[1];
        }
        Host6: HiHost {
            @display("i=device/server;p=1900,2200");
            gates:
                ethg[1];
        }
        Host7: HiHost {
            @display("i=device/server;p=2100,2200");
            gates:
                ethg[1];
        }
        Host8: HiHost {
            @display("i=device/server;p=2300,2200");
            gates:
                ethg[1];
        }
        Host9: HiHost {
            @display("i=device/server;p=2700,2200");
            gates:
                ethg[1];
        }
        Host10: HiHost {
            @display("i=device/server;p=2900,2200");
            gates:
                ethg[1];
        }
        Host11: HiHost {
            @display("i=device/server;p=3100,2200");
            gates:
                ethg[1];
        }
        Host12: HiHost {
            @display("i=device/server;p=3300,2200");
            gates:
                ethg[1];
        }
        Host13: HiHost {
            @display("i=device/server;p=3700,2200");
            gates:
                ethg[1];
        }
        Host14: HiHost {
            @display("i=device/server;p=3900,2200");
            gates:
                ethg[1];
        }
        Host15: HiHost {
            @display("i=device/server;p=4100,2200");
            gates:
                ethg[1];
        }
        Host16: HiHost {
            @display("i=device/server;p=4300,2200");
            gates:
                ethg[1];
        }
    connections:
        Host1.ethg++ <--> Hostchannel <--> Edge1.ethg++;
        Host2.ethg++ <--> Hostchannel <--> Edge1.ethg++;
        Host3.ethg++ <--> Hostchannel <--> Edge2.ethg++;
        Host4.ethg++ <--> Hostchannel <--> Edge2.ethg++;
        Host5.ethg++ <--> Hostchannel <--> Edge3.ethg++;
        Host6.ethg++ <--> Hostchannel <--> Edge3.ethg++;
        Host7.ethg++ <--> Hostchannel <--> Edge4.ethg++;
        Host8.ethg++ <--> Hostchannel <--> Edge4.ethg++;
        Host9.ethg++ <--> Hostchannel <--> Edge5.ethg++;
        Host10.ethg++ <--> Hostchannel <--> Edge5.ethg++;
        Host11.ethg++ <--> Hostchannel <--> Edge6.ethg++;
        Host12.ethg++ <--> Hostchannel <--> Edge6.ethg++;
        Host13.ethg++ <--> Hostchannel <--> Edge7.ethg++;
        Host14.ethg++ <--> Hostchannel <--> Edge7.ethg++;
        Host15.ethg++ <--> Hostchannel <--> Edge8.ethg++;
        Host16.ethg++ <--> Hostchannel <--> Edge8.ethg++;

        Edge1.ethg++ <--> Switchchannel <--> Agre1.ethg++;
        Edge1.ethg++ <--> Switchchannel <--> Agre2.ethg++;
        Edge2.ethg++ <--> Switchchannel <--> Agre1.ethg++;
        Edge2.ethg++ <--> Switchchannel <--> Agre2.ethg++;
        Edge3.ethg++ <--> Switchchannel <--> Agre3.ethg++;
        Edge3.ethg++ <--> Switchchannel <--> Agre4.ethg++;
        Edge4.ethg++ <--> Switchchannel <--> Agre3.ethg++;
        Edge4.ethg++ <--> Switchchannel <--> Agre4.ethg++;
        Edge5.ethg++ <--> Switchchannel <--> Agre5.ethg++;
        Edge5.ethg++ <--> Switchchannel <--> Agre6.ethg++;
        Edge6.ethg++ <--> Switchchannel <--> Agre5.ethg++;
        Edge6.ethg++ <--> Switchchannel <--> Agre6.ethg++;
        Edge7.ethg++ <--> Switchchannel <--> Agre7.ethg++;
        Edge7.ethg++ <--> Switchchannel <--> Agre8.ethg++;
        Edge8.ethg++ <--> Switchchannel <--> Agre7.ethg++;
        Edge8.ethg++ <--> Switchchannel <--> Agre8.ethg++;
        
        Core1.ethg++ <--> Switchchannel <--> Agre1.ethg++;
        Core1.ethg++ <--> Switchchannel <--> Agre3.ethg++;
        Core1.ethg++ <--> Switchchannel <--> Agre5.ethg++;
        Core1.ethg++ <--> Switchchannel <--> Agre7.ethg++;
        Core2.ethg++ <--> Switchchannel <--> Agre1.ethg++;
        Core2.ethg++ <--> Switchchannel <--> Agre3.ethg++;
        Core2.ethg++ <--> Switchchannel <--> Agre5.ethg++;
        Core2.ethg++ <--> Switchchannel <--> Agre7.ethg++;
        Core3.ethg++ <--> Switchchannel <--> Agre2.ethg++;
        Core3.ethg++ <--> Switchchannel <--> Agre4.ethg++;
        Core3.ethg++ <--> Switchchannel <--> Agre6.ethg++;
        Core3.ethg++ <--> Switchchannel <--> Agre8.ethg++;
        Core4.ethg++ <--> Switchchannel <--> Agre2.ethg++;
        Core4.ethg++ <--> Switchchannel <--> Agre4.ethg++;
        Core4.ethg++ <--> Switchchannel <--> Agre6.ethg++;
        Core4.ethg++ <--> Switchchannel <--> Agre8.ethg++;
}
