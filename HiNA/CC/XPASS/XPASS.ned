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

package inet.HiNA.CC.XPASS;

import inet.HiNA.CC.UdpCC;


simple XPASS like UdpCC
{
    parameters:
        bool activate = default(true);
        double stopTime @unit(s) = default(1s);
        double linkspeed @unit(bps) = default (1Gbps);
        int max_pck_size = default(1472);
        double wmax=default(0.5);
        double wmin=default(0.01);
        double initialrate=default(1/16);
        double targetratio=default(0.1);
        //for ECN based control
        double rtt_beta=default(0.5);
        bool useECN = default(false);
        bool useRTT=default(false);
        double thigh=default(0.003);
        double tlow=default(0.001);
        double RTT_a=default(0.5);
        double minRTT=default(0.0001);
        double gamma=default(1/256);
        double alpha=default(1);
        double Rai @unit(bps) = default(40Mbps);
 		double Rhai @unit(bps) = default(400Mbps);
        
        @display("i=block/downarrow,grey80;is=s;i2=status/down");
    gates:
        input upperIn @labels(TCPSegment,IPv4ControlInfo/down,IPv6ControlInfo/down);// higher layer
        output upperOut @labels(TCPSegment,IPv4ControlInfo/up,IPv6ControlInfo/up);
        output lowerOut @labels(TCPSegment,IPv4ControlInfo/down,IPv6ControlInfo/down);
        input lowerIn @labels(TCPSegment,IPv4ControlInfo/up,IPv6ControlInfo/up);
}