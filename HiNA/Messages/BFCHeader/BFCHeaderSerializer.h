/*
 * BFCHeaderSerializer.h
 *
 *  Created on: 2023Äê5ÔÂ26ÈÕ
 *      Author: ergeng2001
 */

#ifndef INET_HINA_MESSAGES_BFCHEADER_BFCHEADERSERIALIZER_H_
#define INET_HINA_MESSAGES_BFCHEADER_BFCHEADERSERIALIZER_H_

#include "inet/common/packet/serializer/FieldsChunkSerializer.h"

namespace inet {

/**
 * Converts between BFCHeader and binary (network byte order) BFC header.
 */
class INET_API BFCHeaderSerializer : public FieldsChunkSerializer
{
  protected:
    virtual void serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const override;
    virtual const Ptr<Chunk> deserialize(MemoryInputStream& stream) const override;

  public:
    BFCHeaderSerializer() : FieldsChunkSerializer() {}
};

} // namespace inet



#endif /* INET_HINA_MESSAGES_BFCHEADER_BFCHEADERSERIALIZER_H_ */
