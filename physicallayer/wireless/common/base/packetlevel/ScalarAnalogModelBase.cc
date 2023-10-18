//
// Copyright (C) 2013 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "inet/physicallayer/wireless/common/base/packetlevel/ScalarAnalogModelBase.h"

#include "inet/common/geometry/common/Quaternion.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarSnir.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IAntennaGain.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadioMedium.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/BandListening.h"

namespace inet {

namespace physicallayer {

void ScalarAnalogModelBase::initialize(int stage)
{
    AnalogModelBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        ignorePartialInterference = par("ignorePartialInterference");
    }
}

bool ScalarAnalogModelBase::areOverlappingBands(Hz centerFrequency1, Hz bandwidth1, Hz centerFrequency2, Hz bandwidth2) const
{
    return centerFrequency1 + bandwidth1 / 2 >= centerFrequency2 - bandwidth2 / 2 &&
           centerFrequency1 - bandwidth1 / 2 <= centerFrequency2 + bandwidth2 / 2;
}

W ScalarAnalogModelBase::computeReceptionPower(const IRadio *receiverRadio, const ITransmission *transmission, const IArrival *arrival) const
{
    const IRadioMedium *radioMedium = receiverRadio->getMedium();
    const INarrowbandSignal *narrowbandSignalAnalogModel = check_and_cast<const INarrowbandSignal *>(transmission->getAnalogModel());
    const IScalarSignal *scalarSignalAnalogModel = check_and_cast<const IScalarSignal *>(transmission->getAnalogModel());
    const Coord& receptionStartPosition = arrival->getStartPosition();
    // TODO could be used for doppler shift? const Coord& receptionEndPosition = arrival->getEndPosition();
    double transmitterAntennaGain = computeAntennaGain(transmission->getTransmitterAntennaGain(), transmission->getStartPosition(), arrival->getStartPosition(), transmission->getStartOrientation());
    double receiverAntennaGain = computeAntennaGain(receiverRadio->getAntenna()->getGain().get(), arrival->getStartPosition(), transmission->getStartPosition(), arrival->getStartOrientation());
    double pathLoss = radioMedium->getPathLoss()->computePathLoss(transmission, arrival);
    double obstacleLoss = radioMedium->getObstacleLoss() ? radioMedium->getObstacleLoss()->computeObstacleLoss(narrowbandSignalAnalogModel->getCenterFrequency(), transmission->getStartPosition(), receptionStartPosition) : 1;
    W transmissionPower = scalarSignalAnalogModel->getPower();
    ASSERT(!std::isnan(transmissionPower.get()));
    double gain = transmitterAntennaGain * receiverAntennaGain * pathLoss * obstacleLoss;
    ASSERT(!std::isnan(gain));
    if (gain > 1.0) {
        EV_WARN << "Signal power attenuation is zero.\n";
        gain = 1.0;
    }
    return transmissionPower * gain;
}

void ScalarAnalogModelBase::addReception(const IReception *reception, simtime_t& noiseStartTime, simtime_t& noiseEndTime, std::map<simtime_t, W> *powerChanges) const
{
    W power = check_and_cast<const IScalarSignal *>(reception->getAnalogModel())->getPower();
    simtime_t startTime = reception->getStartTime();
    simtime_t endTime = reception->getEndTime();
    auto itStartTime = powerChanges->find(startTime);
    if (itStartTime != powerChanges->end())
        itStartTime->second += power;
    else
        powerChanges->insert(std::pair<simtime_t, W>(startTime, power));
    auto itEndTime = powerChanges->find(endTime);
    if (itEndTime != powerChanges->end())
        itEndTime->second -= power;
    else
        powerChanges->insert(std::pair<simtime_t, W>(endTime, -power));
    if (reception->getStartTime() < noiseStartTime)
        noiseStartTime = reception->getStartTime();
    if (reception->getEndTime() > noiseEndTime)
        noiseEndTime = reception->getEndTime();
}

void ScalarAnalogModelBase::addNoise(const ScalarNoise *noise, simtime_t& noiseStartTime, simtime_t& noiseEndTime, std::map<simtime_t, W> *powerChanges) const
{
    const std::map<simtime_t, W> *noisePowerChanges = noise->getPowerChanges();
    for (const auto& noisePowerChange : *noisePowerChanges) {
        auto jt = powerChanges->find(noisePowerChange.first);
        if (jt != powerChanges->end())
            jt->second += noisePowerChange.second;
        else
            powerChanges->insert(std::pair<simtime_t, W>(noisePowerChange.first, noisePowerChange.second));
    }
    if (noise->getStartTime() < noiseStartTime)
        noiseStartTime = noise->getStartTime();
    if (noise->getEndTime() > noiseEndTime)
        noiseEndTime = noise->getEndTime();
}

const INoise *ScalarAnalogModelBase::computeNoise(const IListening *listening, const IInterference *interference) const
{
    const BandListening *bandListening = check_and_cast<const BandListening *>(listening);
    Hz commonCenterFrequency = bandListening->getCenterFrequency();
    Hz commonBandwidth = bandListening->getBandwidth();
    simtime_t noiseStartTime = SimTime::getMaxTime();
    simtime_t noiseEndTime = 0;
    std::map<simtime_t, W> *powerChanges = new std::map<simtime_t, W>();
    const std::vector<const IReception *> *interferingReceptions = interference->getInterferingReceptions();
    for (auto reception : *interferingReceptions) {
        const ISignalAnalogModel *signalAnalogModel = reception->getAnalogModel();
        const INarrowbandSignal *narrowbandSignalAnalogModel = check_and_cast<const INarrowbandSignal *>(signalAnalogModel);
        Hz signalCenterFrequency = narrowbandSignalAnalogModel->getCenterFrequency();
        Hz signalBandwidth = narrowbandSignalAnalogModel->getBandwidth();
        if (commonCenterFrequency == signalCenterFrequency && commonBandwidth >= signalBandwidth)
            addReception(reception, noiseStartTime, noiseEndTime, powerChanges);
        else if (!ignorePartialInterference && areOverlappingBands(commonCenterFrequency, commonBandwidth, narrowbandSignalAnalogModel->getCenterFrequency(), narrowbandSignalAnalogModel->getBandwidth()))
            throw cRuntimeError("Partially interfering signals are not supported by ScalarAnalogModel, enable ignorePartialInterference to avoid this error!");
    }
    const ScalarNoise *scalarBackgroundNoise = dynamic_cast<const ScalarNoise *>(interference->getBackgroundNoise());
    if (scalarBackgroundNoise) {
        if (commonCenterFrequency == scalarBackgroundNoise->getCenterFrequency() && commonBandwidth >= scalarBackgroundNoise->getBandwidth())
            addNoise(scalarBackgroundNoise, noiseStartTime, noiseEndTime, powerChanges);
        else if (!ignorePartialInterference && areOverlappingBands(commonCenterFrequency, commonBandwidth, scalarBackgroundNoise->getCenterFrequency(), scalarBackgroundNoise->getBandwidth()))
            throw cRuntimeError("Partially interfering background noise is not supported by ScalarAnalogModel, enable ignorePartialInterference to avoid this error!");
    }
    EV_TRACE << "Noise power begin " << endl;
    W noise = W(0);
    for (std::map<simtime_t, W>::const_iterator it = powerChanges->begin(); it != powerChanges->end(); it++) {
        noise += it->second;
        EV_TRACE << "Noise at " << it->first << " = " << noise << endl;
    }
    EV_TRACE << "Noise power end" << endl;
    return new ScalarNoise(noiseStartTime, noiseEndTime, commonCenterFrequency, commonBandwidth, powerChanges);
}

const INoise *ScalarAnalogModelBase::computeNoise(const IReception *reception, const INoise *noise) const
{
    auto scalarNoise = check_and_cast<const ScalarNoise *>(noise);
    simtime_t noiseStartTime = SimTime::getMaxTime();
    simtime_t noiseEndTime = 0;
    std::map<simtime_t, W> *powerChanges = new std::map<simtime_t, W>();
    addReception(reception, noiseStartTime, noiseEndTime, powerChanges);
    addNoise(scalarNoise, noiseStartTime, noiseEndTime, powerChanges);
    return new ScalarNoise(noiseStartTime, noiseEndTime, scalarNoise->getCenterFrequency(), scalarNoise->getBandwidth(), powerChanges);
}

const ISnir *ScalarAnalogModelBase::computeSNIR(const IReception *reception, const INoise *noise) const
{
    return new ScalarSnir(reception, noise);
}

} // namespace physicallayer

} // namespace inet

