import FWCore.ParameterSet.Config as cms

electronPixelSeedConfiguration = cms.PSet(
    searchInTIDTEC = cms.bool(True), ##  possibility to inhibit extended forward coverage

    HighPtThreshold = cms.double(35.0),
    r2MinF = cms.double(-0.15),
    DeltaPhi1Low = cms.double(0.23),
    DeltaPhi1High = cms.double(0.08),
    # non dynamic (overwritten in case of dynamicPhiRoad)
    ePhiMin1 = cms.double(-0.125),
    # general
    PhiMin2 = cms.double(-0.002),
    # dynamicPhiRoad
    LowPtThreshold = cms.double(5.0),
    maxHOverE = cms.double(0.2),
    dynamicPhiRoad = cms.bool(True),
    ePhiMax1 = cms.double(0.075),
    DeltaPhi2 = cms.double(0.004),
    SizeWindowENeg = cms.double(0.675),
    rMaxI = cms.double(0.2), ## intermediate region SC in EB and hits in PXF

    PhiMax2 = cms.double(0.002),
    hbheInstance = cms.string(''),
    pPhiMin1 = cms.double(-0.075),
    pPhiMax1 = cms.double(0.125),
    # for filtering
    hbheModule = cms.string('hbhereco'),
    SCEtCut = cms.double(5.0),
    z2MaxB = cms.double(0.09),
    z2MinB = cms.double(-0.09),
    r2MaxF = cms.double(0.15),
    rMinI = cms.double(-0.2) ## intermediate region SC in EB and hits in PXF

)

