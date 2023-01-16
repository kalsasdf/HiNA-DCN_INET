//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import inet.linklayer.ethernet.common.EthernetControlFrame;

namespace inet;

enum EthernetPfcControlOpCode
{
    ETHERNET_PFC_PAUSE = 3;
    ETHERNET_PFC_RESUME = 4;
}

class EthernetPfcFrame extends EthernetControlFrameBase
{
    chunkLength = B(20); 
    int16_t priority = -1;
    int16_t pauseTime = 100; // in 512 bit-time units
}
