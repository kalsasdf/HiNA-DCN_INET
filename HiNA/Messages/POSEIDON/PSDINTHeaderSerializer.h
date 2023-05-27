//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_PSDINTHEADERSERIALIZER_H
#define __INET_PSDINTHEADERSERIALIZER_H

#include "inet/common/packet/serializer/FieldsChunkSerializer.h"

namespace inet {

/**
 * Converts between INTHeader and binary (network byte order) INT header.
 */
class INET_API PSDINTHeaderSerializer : public FieldsChunkSerializer
{
  protected:
    virtual void serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const override;
    virtual const Ptr<Chunk> deserialize(MemoryInputStream& stream) const override;

  public:
    PSDINTHeaderSerializer() : FieldsChunkSerializer() {}
};

} // namespace inet

#endif

