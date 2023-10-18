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

package inet.HiNA.CC.TIMELY;

import inet.HiNA.CC.UdpCC;
//
// TODO auto-generated type
//
simple TIMELY like UdpCC
{
    parameters:
        bool activate = default(true);
        double linkspeed @unit(bps) = default (10Gbps);
        double stopTime @unit(s) = default(10s);
        double Rai @unit(bps) = default(10Mbps);  // 最大速率的 1 / 50
 		double minRTT @unit(s)= default(20us);
 		double Tlow @unit(s)= default(50us);  // 修改Tlow
 		double Thigh @unit(s)= default(500us);
 		double alpha = default(0.875);
 		double beta = default(0.8);
 		int max_pck_size = default(1472);
 		int TIMELYseg @unit(B)= default(16kB);
        
        @display("i=block/downarrow,grey80;is=s;i2=status/down");
    gates:
        input upperIn @labels(UdpHeader,IPv4ControlInfo/down,IPv6ControlInfo/down);// higher layer
        output upperOut @labels(UdpHeader,IPv4ControlInfo/up,IPv6ControlInfo/up);
        output lowerOut @labels(UdpHeader,IPv4ControlInfo/down,IPv6ControlInfo/down);
        input lowerIn @labels(UdpHeader,IPv4ControlInfo/up,IPv6ControlInfo/up);
}
