import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

namespace inet;

class CNP extends FieldsChunk
{
    chunkLength=b(32);
    uint32_t RecRate;
}