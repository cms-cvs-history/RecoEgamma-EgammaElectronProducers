// -*- C++ -*-
//
// Package:    ElectronProducers
// Class:      ElectronSeedProducer
//
/**\class ElectronSeedProducer RecoEgamma/ElectronProducers/src/ElectronSeedProducer.cc

 Description: EDProducer of ElectronSeed objects

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Ursula Berthon, Claude Charlot
//         Created:  Mon Mar 27 13:22:06 CEST 2006
// $Id: ElectronSeedProducer.cc,v 1.5 2009/05/04 06:33:01 chamont Exp $
//
//

#include "ElectronSeedProducer.h"

#include "RecoEgamma/EgammaIsolationAlgos/interface/EgammaHcalIsolation.h"
//#include "DataFormats/RecoCandidate/interface/RecoCandidate.h"

#include "RecoEgamma/EgammaElectronAlgos/interface/ElectronSeedGenerator.h"
//#include "RecoEgamma/EgammaElectronAlgos/interface/ElectronHcalHelper.h"
#include "RecoEgamma/EgammaElectronAlgos/interface/SeedFilter.h"

#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "Geometry/CaloGeometry/interface/CaloSubdetectorGeometry.h"
#include "RecoCaloTools/Selectors/interface/CaloConeSelector.h"

#include "DataFormats/EgammaReco/interface/ElectronSeed.h"
#include "DataFormats/EgammaReco/interface/ElectronSeedFwd.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include <string>

using namespace reco;

ElectronSeedProducer::ElectronSeedProducer( const edm::ParameterSet& iConfig )
 :
   //conf_(iConfig),
   seedFilter_(0),
   hcalIso_(0),
   //doubleConeSel_(0),
   mhbhe_(0),
   cacheID_(0)

 {
  edm::ParameterSet pset = iConfig.getParameter<edm::ParameterSet>("SeedConfiguration") ;

  initialSeeds_ = pset.getParameter<edm::InputTag>("initialSeeds") ;
  SCEtCut_ = pset.getParameter<double>("SCEtCut") ;
  fromTrackerSeeds_ = pset.getParameter<bool>("fromTrackerSeeds") ;
  prefilteredSeeds_ = pset.getParameter<bool>("preFilteredSeeds") ;

  // for H/E
  hcalRecHits_ = pset.getParameter<edm::InputTag>("hcalRecHits") ;
  maxHOverE_=pset.getParameter<double>("maxHOverE") ;
  hOverEConeSize_=pset.getParameter<double>("hOverEConeSize") ;
  hOverEHBMinE_=pset.getParameter<double>("hOverEHBMinE") ;
  hOverEHFMinE_=pset.getParameter<double>("hOverEHFMinE") ;

  matcher_ = new ElectronSeedGenerator(pset) ;
  //hcalHelper_ = new ElectronHcalHelper(pset) ;

  if (prefilteredSeeds_) seedFilter_ = new SeedFilter(pset) ;

  //  get collections from config'
  superClusters_[0]=iConfig.getParameter<edm::InputTag>("barrelSuperClusters") ;
  superClusters_[1]=iConfig.getParameter<edm::InputTag>("endcapSuperClusters") ;

  //register your products
  produces<ElectronSeedCollection>() ;
}


ElectronSeedProducer::~ElectronSeedProducer()
 {
  //delete hcalHelper_ ;
  delete matcher_ ;
  delete seedFilter_ ;
  delete mhbhe_ ;
  //delete doubleConeSel_ ;
  delete hcalIso_ ;
 }

void ElectronSeedProducer::produce(edm::Event& e, const edm::EventSetup& iSetup)
 {
  LogDebug("ElectronSeedProducer")  <<"[ElectronSeedProducer::produce] entering " ;

  //hcalHelper_->checkSetup(iSetup) ;
  //hcalHelper_->readEvent(e) ;

  // get calo geometry
  if (cacheID_!=iSetup.get<CaloGeometryRecord>().cacheIdentifier()) {
    iSetup.get<CaloGeometryRecord>().get(theCaloGeom);
    cacheID_=iSetup.get<CaloGeometryRecord>().cacheIdentifier();
  }

  matcher_->setupES(iSetup);

  // get Hcal Rechit collection
  edm::Handle<HBHERecHitCollection> hbhe ;
  bool got = e.getByLabel(hcalRecHits_,hbhe) ;
  delete mhbhe_;
  if (got) { mhbhe_=  new HBHERecHitMetaCollection(*hbhe) ; }
  else { mhbhe_ = 0 ; }

  // define cone for H/E
  //delete doubleConeSel_;
  //doubleConeSel_ = new CaloDualConeSelector(0.,hOverEConeSize_,theCaloGeom.product(),DetId::Hcal);

  // get initial TrajectorySeeds if necessary
  if (fromTrackerSeeds_) {
    if (!prefilteredSeeds_) {
      edm::Handle<TrajectorySeedCollection> hSeeds;
      e.getByLabel(initialSeeds_, hSeeds);
      theInitialSeedColl = const_cast<TrajectorySeedCollection *> (hSeeds.product());
    }
    else theInitialSeedColl =new TrajectorySeedCollection;
  }else
    theInitialSeedColl=0;// not needed in this case

  ElectronSeedCollection *seeds= new ElectronSeedCollection;

  // HCAL iso deposits
  delete hcalIso_;
  hcalIso_ = new EgammaHcalIsolation(hOverEConeSize_,0.,hOverEHBMinE_,hOverEHFMinE_,0.,0.,theCaloGeom,mhbhe_) ;

  // loop over barrel + endcap
  for (unsigned int i=0; i<2; i++) {
   // invoke algorithm
    edm::Handle<SuperClusterCollection> clusters;
    if (e.getByLabel(superClusters_[i],clusters))   {
	SuperClusterRefVector clusterRefs;
	filterClusters(clusters,mhbhe_,clusterRefs);
	if ((fromTrackerSeeds_) && (prefilteredSeeds_)) filterSeeds(e,iSetup,clusterRefs);
        matcher_->run(e,iSetup,clusterRefs,theInitialSeedColl,*seeds);

    }
  }

  // store the accumulated result
  std::auto_ptr<ElectronSeedCollection> pSeeds(seeds) ;
  ElectronSeedCollection::iterator is ;
  for ( is=pSeeds->begin() ; is!=pSeeds->end() ; is++ )
   {
    edm::RefToBase<CaloCluster> caloCluster = is->caloCluster() ;
    SuperClusterRef superCluster = caloCluster.castTo<SuperClusterRef>() ;
    LogDebug("ElectronSeedProducer")<< "new seed with "
      << (*is).nHits() << " hits"
      << ", charge " << (*is).getCharge()
      << " and cluster energy " << superCluster->energy()
      << " PID "<<superCluster.id() ;
   }
  e.put(pSeeds) ;
  if (fromTrackerSeeds_ && prefilteredSeeds_) delete theInitialSeedColl;
 }


//===============================
// Filter the superclusters
// - with EtCut
// - with HoE using calo cone
//===============================

void ElectronSeedProducer::filterClusters
 ( const edm::Handle<reco::SuperClusterCollection> & superClusters,
   HBHERecHitMetaCollection * mhbhe, SuperClusterRefVector & sclRefs )
 {
  for (unsigned int i=0;i<superClusters->size();++i)
   {
    const SuperCluster & scl = (*superClusters)[i] ;
    if (scl.energy()/cosh(scl.eta())>SCEtCut_)
     {
//      double hcalE = 0. ;
//      if (mhbhe_)
//       {
//	    math::XYZPoint theCaloPosition = scl.position() ;
//	    GlobalPoint pclu(theCaloPosition.x(),theCaloPosition.y(),theCaloPosition.z()) ;
//	    std::auto_ptr<CaloRecHitMetaCollectionV> chosen = doubleConeSel_->select(pclu,*mhbhe_) ;
//	    CaloRecHitMetaCollectionV::const_iterator i ;
//	    for ( i=chosen->begin() ; i!=chosen->end() ; ++i )
//	     {
//	      double hcalHit_E = i->energy() ;
//	      if ( i->detid().subdetId()==HcalBarrel && hcalHit_E > hOverEHBMinE_) hcalE += hcalHit_E; //HB case
//	      //if ( i->detid().subdetId()==HcalBarrel) {
//	      //std::cout << "[ElectronSeedProducer] HcalBarrel: hcalHit_E, hOverEHBMinE_ " << hcalHit_E << " " << hOverEHBMinE_ << std::endl;
//	      //}
//	      if ( i->detid().subdetId()==HcalEndcap && hcalHit_E > hOverEHFMinE_) hcalE += hcalHit_E; //HF case
//	      //if ( i->detid().subdetId()==HcalEndcap) {
//	      //std::cout << "[ElectronSeedProducer] HcalEndcap: hcalHit_E, hOverEHFMinE_ " << hcalHit_E << " " << hOverEHFMinE_ << std::endl;
//	      //}
//	     }
//       }
//      double HoE = hcalE/scl.energy() ;
      //double hcalE = hcalHelper_->hcalESum(scl), HoE = hcalE/scl.energy() ;
      double newHcalE = hcalIso_->getHcalESum(&scl), newHoE = newHcalE/scl.energy() ;
      //std::cout << "[ElectronSeedProducer] HoE, maxHOverE_ " << newHoE << " " << HoE << " " << maxHOverE_ << std::endl ;
      if (newHoE <= maxHOverE_)
       { sclRefs.push_back(edm::Ref<reco::SuperClusterCollection> (superClusters,i)) ; }
     }
   }
  LogDebug("ElectronSeedProducer")<<"Filtered out "<<sclRefs.size()<<" superclusters from "<<superClusters->size() ;
 }

void ElectronSeedProducer::filterSeeds
 ( edm::Event & event, const edm::EventSetup & setup,
   reco::SuperClusterRefVector & sclRefs )
 {
  for ( unsigned int i=0 ; i<sclRefs.size() ; ++i )
   {
    seedFilter_->seeds(event,setup,sclRefs[i],theInitialSeedColl) ;
    LogDebug("ElectronSeedProducer")<<"Number of Seeds: "<<theInitialSeedColl->size() ;
   }
 }
