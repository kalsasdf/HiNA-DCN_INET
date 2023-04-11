//
// Copyright (C) 2018 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

//
//#include "INTHeaderDissector.h"
//
//#include "inet/common/ProtocolGroup.h"
//#include "inet/common/packet/dissector/ProtocolDissectorRegistry.h"
//#include "inet/transportlayer/udp/Udp.h"
//#include "INTHeader_m.h"
//
//namespace inet {
//
//Register_Protocol_Dissector(&Protocol::hpcc, INTHeaderDissector);
//
//void INTHeaderDissector::dissect(Packet *packet, const Protocol *protocol, ICallback& callback) const
//{
//    auto originalTrailerPopOffset = packet->getBackOffset();
//    auto intHeaderOffset = packet->getFrontOffset();EV<<""<<endl;
//    auto header = packet->popAtFront<INTHeader>();
//    callback.startProtocolDataUnit(&Protocol::hpcc);
//
//    callback.visitChunk(header, &Protocol::hpcc);
//    auto intPayloadEndOffset = intHeaderOffset + B(42);
//
//    packet->setFrontOffset(intPayloadEndOffset);
//    packet->setBackOffset(originalTrailerPopOffset);
//    callback.endProtocolDataUnit(&Protocol::hpcc);
//}
//
//} // namespace inet

