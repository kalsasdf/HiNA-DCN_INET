//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_NODEBASE_H
#define __INET_NODEBASE_H

#include "inet/common/INETDefs.h"

namespace inet {

class INET_API HiNodeBase : public cModule
{
  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
};

} // namespace inet

#endif

