//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "PSDINTHeaderSerializer.h"

#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"
#include "PSDINTHeader_m.h"

namespace inet {

Register_Serializer(PSDINTHeader, PSDINTHeaderSerializer);

void PSDINTHeaderSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    const auto& intHeader = staticPtrCast<const PSDINTHeader>(chunk);
    stream.writeByte(intHeader->getTS().dbl());
    stream.writeByte(intHeader->getMPD().dbl());
    //stream.writeUint8(intHeader->getTxRate());
//    stream.writeUint24Be(intHeader->getTS2().dbl());
//    stream.writeUint16Be(intHeader->getTxBytes());

}

const Ptr<Chunk> PSDINTHeaderSerializer::deserialize(MemoryInputStream& stream) const
{
    auto intHeader = makeShared<PSDINTHeader>();
    intHeader->setTS(simtime_t(stream.readByte()));
    intHeader->setMPD(simtime_t(stream.readByte()));
    //intHeader->setTxRate(double(stream.readUint8()));
//    intHeader->setTS2(simtime_t(stream.readUint24Be()));
//    intHeader->setTxBytes(stream.readUint16Be());

    return intHeader;
}

} // namespace inet

