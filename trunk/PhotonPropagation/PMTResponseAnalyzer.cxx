// \file PMTResponseAnalyzer.cxx  
// \author Ben Jones, MIT 2010
//
// Module to determine how many phots have been detected at each PMT
//
// This analyzer takes the PMTHitCollection generated by LArG4's sensitive detectors
// and fills up to four trees in the histograms file.  The four trees are:
// 
// PMTEvents       - count how many phots hit the PMT face / were detected across all PMT's per event
// PMTs            - count how many phots hit the PMT face / were detected in each PMT individually for each event
// AllPhotons      - wavelength information for each phot hitting the PMT face
// DetectedPhotons - wavelength information for each phot detected
//
// The user may supply a quantum efficiency and sensitive wavelength range for the PMT's.
// with a QE < 1 and a finite wavelength range, a "detected" phot is one which is
// in the relevant wavelength range and passes the random sampling condition imposed by
// the quantum efficiency of the PMT
//
// PARAMETERS REQUIRED:
// int32   Verbosity          - whether to write to screen a well as to file. levels 0 to 3 specify different levels of detail to display
// string  InputModule        - the module which produced the PMTHitCollection
// bool    MakeAllPhotonsTree - whether to build and store each tree (performance can be enhanced by switching off those not required)
// bool    MakeDetectedPhotonsTree
// bool    MakePMTHitsTree
// bool    MakeEventsTree  
// double  QantumEfficiency   - Quantum efficiency of PMT
// double  WavelengthCutLow   - Sensitive wavelength range of PMT 
// double  WavelengthCutHigh 


#ifndef __CINT__

#include "PhotonPropagation/PMTResponseAnalyzer.h"

// LArSoft includes
#include "Simulation/sim.h"

// FMWK includes
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Persistency/Common/Ptr.h"
#include "art/Persistency/Common/PtrVector.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "PhotonPropagation/PMTResponseAnalyzer.h"
#include "Simulation/SimListUtils.h"

// ROOT includes
#include <TH1D.h>
#include <TF1.h>
#include <TTree.h>

// C++ language includes
#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>

#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Random/RandGaussQ.h"

// Debug flag; only used during code development.
const bool debug = true;

namespace phot {
  

  PMTResponseAnalyzer::PMTResponseAnalyzer(fhicl::ParameterSet const& pset)
  {
    fVerbosity=                pset.get<int>("Verbosity");
    fInputModule=              pset.get<std::string>("InputModule");
    fMakeAllPhotonsTree=       pset.get<bool>("MakeAllPhotonsTree");
    fMakeDetectedPhotonsTree=  pset.get<bool>("MakeDetectedPhotonsTree");
    fMakePMTHitsTree=          pset.get<bool>("MakePMTHitsTree");
    fMakeEventsTree=           pset.get<bool>("MakeEventsTree");
    fQE=                       pset.get<double>("QuantumEfficiency");
    fWavelengthCutLow=         pset.get<double>("WavelengthCutLow");
    fWavelengthCutHigh=        pset.get<double>("WavelengthCutHigh");

    // get the random number seed, use a random default if not specified    
    // in the configuration file.  
    unsigned int seed = pset.get< unsigned int >("Seed", sim::GetRandomNumberSeed());
    createEngine(seed);    
  }
  

  void PMTResponseAnalyzer::beginJob()
  {
    // Get file service to store trees
    art::ServiceHandle<art::TFileService> tfs;

     // Create and assign branch addresses to required tree
    if(fMakeAllPhotonsTree)
      {
	fThePhotonTreeAll = tfs->make<TTree>("AllPhotons","AllPhotons");
	fThePhotonTreeAll->Branch("EventID",     &fEventID,          "EventID/I");
	fThePhotonTreeAll->Branch("Wavelength",  &fWavelength,       "Wavelength/F");
	fThePhotonTreeAll->Branch("PMTID",       &fPMTID,            "PMTID/I");
	fThePhotonTreeAll->Branch("Time",        &fTime,             "Time/F");
      }

    if(fMakeDetectedPhotonsTree)
      {
	fThePhotonTreeDetected = tfs->make<TTree>("DetectedPhotons","DetectedPhotons");
	fThePhotonTreeDetected->Branch("EventID",     &fEventID,          "EventID/I");
	fThePhotonTreeDetected->Branch("Wavelength",  &fWavelength,       "Wavelength/F");
	fThePhotonTreeDetected->Branch("PMTID",       &fPMTID,            "PMTID/I");
	fThePhotonTreeDetected->Branch("Time",        &fTime,             "Time/F");
      }

    if(fMakePMTHitsTree)
      {
	fThePMTTree    = tfs->make<TTree>("PMTs","PMTs");
	fThePMTTree->Branch("EventID",        &fEventID,          "EventID/I");
	fThePMTTree->Branch("PMTID",          &fPMTID,            "PMTID/I");
	fThePMTTree->Branch("CountAll",       &fCountPMTAll,      "CountAll/I");
	fThePMTTree->Branch("CountDetected",  &fCountPMTDetected, "CountDetected/I");
      }
    
    if(fMakeEventsTree)
      {
	fTheEventTree  = tfs->make<TTree>("PMTEvents","PMTEvents");
	fTheEventTree->Branch("EventID",      &fEventID,            "EventID/I");
	fTheEventTree->Branch("CountAll",     &fCountEventAll,     "CountAll/I");
	fTheEventTree->Branch("CountDetected",&fCountEventDetected,"CountDetected/I");
      }

  }

  
  PMTResponseAnalyzer::~PMTResponseAnalyzer() 
  {
  }
  
  void PMTResponseAnalyzer::analyze(art::Event const& evt)
  {

    // Setup random number generator (for QE sampling)
    art::ServiceHandle<art::RandomNumberGenerator> rng;
    CLHEP::HepRandomEngine &engine = rng->getEngine();
    CLHEP::RandFlat   flat(engine);

    // Lookup event ID from event
    art::EventNumber_t event = evt.id().event();
    fEventID=Int_t(event);

    //Get PMTHitCollection from Event
    art::ServiceHandle<sim::SimListUtils> slu;
    sim::PMTHitCollection TheHitCollection = slu->GetPMTHitCollection();

    //Reset counters
    fCountEventAll=0;
    fCountEventDetected=0;

    if(fVerbosity > 0) std::cout<<"Found PMT hit collection of size "<< TheHitCollection.size()<<std::endl;
    if(TheHitCollection.size()>0)
      {
	for(sim::PMTHitCollection::const_iterator it=TheHitCollection.begin(); it!=TheHitCollection.begin(); it++)
	  {
	    //Reset Counters
	    fCountPMTAll=0;
	    fCountPMTDetected=0;

	    //Get data from HitCollection entry
	    fPMTID=it->first;
	    const sim::PMTHit * TheHit=it->second;
	      
	    // Loop through PMT phots.  
	    //   Note we make the screen output decision outside the loop
	    //   in order to avoid evaluating large numbers of unnecessary 
	    //   if conditions. 

	    if(fVerbosity > 3)
	      {
		for(sim::PMTHit::const_iterator it = TheHit->begin(); it!=TheHit->end(); it++)
		  {
		    // Calculate wavelength in nm
		    fWavelength= (2.0*3.142)*0.000197/it->Momentum.T();

		    //Get arrival time from phot
		    fTime= it->Position.T();

		    // Increment per PMT counters and fill per phot trees
		    fCountPMTAll++;
		    if(fMakeAllPhotonsTree) fThePhotonTreeAll->Fill();
		    if((flat.fire(1.0)<=fQE)&&(fWavelength>fWavelengthCutLow)&&(fWavelength<fWavelengthCutHigh))
		      {
			if(fMakeDetectedPhotonsTree) fThePhotonTreeDetected->Fill();
			fCountPMTDetected++;
			std::cout<<"PMTResponse PerPhoton : Event "<<fEventID<<" PMTID " <<fPMTID << " Wavelength " << fWavelength << " Detected 1 "<<std::endl;
		      }
		    else
		      std::cout<<"PMTResponse PerPhoton : Event "<<fEventID<<" PMTID " <<fPMTID << " Wavelength " << fWavelength << " Detected 0 "<<std::endl;
		  }
	      }
	    else
	      {
		for(sim::PMTHit::const_iterator it = TheHit->begin(); it!=TheHit->end(); it++)
		  {
		    // Calculate wavelength in nm
		    fWavelength= (2.0*3.142)*0.000197/it->Momentum.T();
		    
		    // Increment per PMT counters and fill per phot trees
		    fCountPMTAll++;
		    if(fMakeAllPhotonsTree) fThePhotonTreeAll->Fill();
		    if((flat.fire(1.0)<=fQE)&&(fWavelength>fWavelengthCutLow)&&(fWavelength<fWavelengthCutHigh))
		      {
			if(fMakeDetectedPhotonsTree) fThePhotonTreeDetected->Fill();
			fCountPMTDetected++;
		      }
		  }
	      }

	  	      
	    // Incremenent per event and fill Per PMT trees
	    
	    if(fMakePMTHitsTree) fThePMTTree->Fill();
	    fCountEventAll+=fCountPMTAll;
	    fCountEventDetected+=fCountPMTDetected;

	    // Give per PMT output
	    if(fVerbosity >2) std::cout<<"PMTResponse PerPMT : Event "<<fEventID<<" PMT " << fPMTID << " All " << fCountPMTAll << " Det " <<fCountPMTDetected<<std::endl; 
	  }

	// Fill per event tree
	if(fMakeEventsTree) fTheEventTree->Fill();

	// Give per event output
	if(fVerbosity >1) std::cout<<"PMTResponse PerEvent : Event "<<fEventID<<" All " << fCountPMTAll << " Det " <<fCountPMTDetected<<std::endl; 	

      }
    else
      {
	// if empty PMT hit collection, 
	// add an empty record to the per event tree 
	fTheEventTree->Fill();
      }
    
  }
  
}

#endif
