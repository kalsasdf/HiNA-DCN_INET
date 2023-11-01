/*
 * BFCHeaderSerializer.cc
 *
 *  Created on: 2023年5月26日
 *      Author: ergeng2001
 */



#include "BFCHeaderSerializer.h"

#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"
#include "BFCHeader_m.h"

namespace inet {

Register_Serializer(BFCHeader, BFCHeaderSerializer);

void BFCHeaderSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    const auto& bfcHeader = staticPtrCast<const BFCHeader>(chunk);
    stream.writeUint32Be(bfcHeader->getQueueID());//int类型
    stream.writeUint32Be(bfcHeader->getUpstreamQueueID());


}

const Ptr<Chunk> BFCHeaderSerializer::deserialize(MemoryInputStream& stream) const
{
    auto bfcHeader = makeShared<BFCHeader>();

    bfcHeader->setQueueID(stream.readUint32Be());
    bfcHeader->setUpstreamQueueID(stream.readUint32Be());

    return bfcHeader;
}

} // namespace inet
