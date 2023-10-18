//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "HiNodeBase.h"

#include "inet/common/SubmoduleLayout.h"

namespace inet {

Define_Module(HiNodeBase);

void HiNodeBase::initialize(int stage)
{
    if (stage == INITSTAGE_LAST)
        layoutSubmodulesWithoutGates(this);
}

} // namespace inet

