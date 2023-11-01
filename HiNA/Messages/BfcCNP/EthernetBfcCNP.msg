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
import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import inet.linklayer.ethernet.common.EthernetControlFrame;

namespace inet;

enum EthernetBfcCNPOpCode
{
    ETHERNET_BFC_DECELERATION = 5;
    ETHERNET_BFC_SPEEDUP = 6;
}

class EthernetBfcCNPFrame extends EthernetControlFrameBase
{
    chunkLength = B(20); 
    int16_t queueID = -1;
    int16_t pauseTime = 100; // in 512 bit-time units
}
