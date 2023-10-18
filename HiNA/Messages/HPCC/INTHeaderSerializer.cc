//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "INTHeaderSerializer.h"

#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"
#include "INTHeader_m.h"

namespace inet {

Register_Serializer(INTHeader, INTHeaderSerializer);

void INTHeaderSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    const auto& intHeader = staticPtrCast<const INTHeader>(chunk);
    stream.writeUint8(intHeader->getNHop());
    stream.writeUint8(intHeader->getPathID());
    for(int i=0;i<5;i++){
        hopInf si = intHeader->getHopInfs(i);
        stream.writeUint8(si.txRate);
        stream.writeUint24Be(si.TS.dbl());
        stream.writeUint16Be(si.txBytes);
        stream.writeUint16Be(si.queueLength.get());
    }

}

const Ptr<Chunk> INTHeaderSerializer::deserialize(MemoryInputStream& stream) const
{
    auto intHeader = makeShared<INTHeader>();
    intHeader->setNHop(stream.readUint8());
    intHeader->setPathID(stream.readUint8());
    int length = stream.getRemainingLength().get();
    int counter = 0;
    for(int i=0;i<length;i+=64){
        hopInf si;
        si.txRate=double(stream.readUint8());
        si.TS=simtime_t(stream.readUint24Be());
        si.txBytes=stream.readUint16Be();
        si.queueLength=b(stream.readUint16Be());
        intHeader->setHopInfs(counter++,si);
    }
    return intHeader;
}

} // namespace inet

