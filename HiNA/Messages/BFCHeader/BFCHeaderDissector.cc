///*
// * BFCHeaderDissector.cc
// *
// *  Created on: 2023Äê5ÔÂ26ÈÕ
// *      Author: ergeng2001
// */
//#include "BFCHeaderDissector.h"
//
//#include "inet/common/ProtocolGroup.h"
//#include "inet/common/packet/dissector/ProtocolDissectorRegistry.h"
//#include "inet/transportlayer/udp/Udp.h"
//#include "BFCHeader_m.h"
//
//#include "inet/networklayer/ipv4/Ipv4ProtocolDissector.h"
//
//#include "inet/common/ProtocolTag_m.h"
//#include "inet/common/packet/dissector/ProtocolDissectorRegistry.h"
//#include "inet/networklayer/ipv4/Ipv4.h"
//
//namespace inet {
//
//Register_Protocol_Dissector(&Protocol::ethernetBFC, BFCHeaderDissector);
//
//void BFCHeaderDissector::dissect(Packet *packet, const Protocol *protocol, ICallback& callback) const
//{
//    auto originalTrailerPopOffset = packet->getBackOffset();
//    auto bfcHeaderOffset = packet->getFrontOffset();EV<<"  "<<endl;
//    auto header = packet->popAtFront<BFCHeader>();
//    callback.startProtocolDataUnit(&Protocol::ethernetBFC);
//
//    callback.visitChunk(header, &Protocol::ethernetBFC);
//    auto bfcPayloadEndOffset = bfcHeaderOffset + B(2);
//
//    packet->setFrontOffset(bfcPayloadEndOffset);
//    packet->setBackOffset(originalTrailerPopOffset);
//    callback.endProtocolDataUnit(&Protocol::ethernetBFC);
//}
//
//} // namespace inet
//
//
//
//
