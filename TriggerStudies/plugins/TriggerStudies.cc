// -*- C++ -*-
//
// Package:    MyStudies/TriggerStudies
// Class:      TriggerStudies
// 
/**\class TriggerStudies TriggerStudies.cc MyStudies/TriggerStudies/plugins/TriggerStudies.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Emanuele Usai
//         Created:  Fri, 10 Oct 2014 09:59:19 GMT
//
//


// system include files
#include <memory>
#include <string>
#include <TLorentzVector.h>
#include "TH2.h"
#include "TH1.h"

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "HLTrigger/HLTcore/interface/HLTConfigProvider.h"
#include "DataFormats/HLTReco/interface/TriggerEvent.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "FWCore/Framework/interface/TriggerNamesService.h"
#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/METReco/interface/PFMET.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/JetReco/interface/PFJetCollection.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenRunInfoProduct.h"

#include "DataFormats/MuonReco/interface/MuonSelectors.h"


using namespace edm;
using namespace reco;

//
// class declaration
//

class TriggerStudies : public edm::EDAnalyzer {
   public:
      explicit TriggerStudies(const edm::ParameterSet&);
      ~TriggerStudies();

      static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
      static bool compare_JetMass(const TLorentzVector jet1, const TLorentzVector jet2){
	return ( jet1.M() > jet2.M() );}
      static bool compare_JetPt(const TLorentzVector jet1, const TLorentzVector jet2){
	return ( jet1.Pt() > jet2.Pt() );}


   private:
      virtual void beginJob() override;
      virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
      virtual void endJob() override;

      // ----------member data ---------------------------
      std::map< std::string, TH1D* > histos1D_;
      
      edm::InputTag jets_inp_;
      edm::InputTag muon_inp_;
      edm::InputTag ele_inp_;
      edm::InputTag met_inp_;
      std::string processName_;
      std::string triggerPath_;
      std::vector<std::string> triggerPathComb_;
      double jet_lead_pt_cut_;
      double jet_subl_pt_cut_;
      double jet_eta_;
      double muon_pt_cut_;
      double ele_pt_cut_;
      double st_muon_pt_;
      double st_ele_pt_;
      double st_lead_jet_pt_;
      int mode_;  // muon: 0, electron: 1


      int triggerBit_;
      std::vector<int> triggerBitsComb_;

      HLTConfigProvider hltConfig_;
};

//
// constructors and destructor
//
TriggerStudies::TriggerStudies(const edm::ParameterSet& iConfig)
{
   processName_     = iConfig.getParameter<std::string> ( "process_name" );
   triggerPath_     = iConfig.getParameter<std::string> ( "triggerpath" );
   triggerPathComb_ = iConfig.getParameter<std::vector<std::string>> ( "triggerpathcomb" );
   jets_inp_	    = iConfig.getParameter<edm::InputTag> ( "jets_inp" );
   muon_inp_	    = iConfig.getParameter<edm::InputTag> ( "muon_inp" );
   ele_inp_	        = iConfig.getParameter<edm::InputTag> ( "ele_inp" );
   met_inp_	        = iConfig.getParameter<edm::InputTag> ( "met_inp" );
   jet_lead_pt_cut_ = iConfig.getParameter<double> ( "jet_lead_pt" );
   jet_subl_pt_cut_ = iConfig.getParameter<double> ( "jet_subl_pt" );
   jet_eta_         = iConfig.getParameter<double> ( "jet_eta" );
   muon_pt_cut_     = iConfig.getParameter<double> ( "muon_pt_cut" );
   ele_pt_cut_      = iConfig.getParameter<double> ( "ele_pt_cut" );
   st_muon_pt_      = iConfig.getParameter<double> ( "st_muon_pt" );
   st_ele_pt_       = iConfig.getParameter<double> ( "st_ele_pt" );
   st_lead_jet_pt_  = iConfig.getParameter<double> ( "st_lead_jet_pt" );
   mode_            = iConfig.getParameter<int> ( "mode" );
}


TriggerStudies::~TriggerStudies()
{}

//
// member functions
//

static bool accept_ele(const reco::GsfElectron& e)
{
    return fabs(e.eta()) < 1.479 ? (                     // Barrel
      fabs(1 - e.eSuperClusterOverP())/e.ecalEnergy() < 0.05 &&
      fabs(e.deltaEtaSuperClusterTrackAtVtx()) < 0.009 &&
      fabs(e.deltaPhiSuperClusterTrackAtVtx()) < 0.1 &&
      e.sigmaIetaIeta() < 0.01 &&
      e.hadronicOverEm() < 0.1  // &&
//      e.dr03EcalRecHitSumEt()/e.pt() < 0.2 &&
//      e.dr03HcalTowerSumEt()/e.pt() < 0.2 &&
//      e.dr03TkSumPt()/e.pt() < 0.2
    ):(                                         // Endcap
      fabs(e.eta()) < 2.5 &&
      fabs(1 - e.eSuperClusterOverP())/e.ecalEnergy() < 0.05 &&
      fabs(e.deltaEtaSuperClusterTrackAtVtx()) < 0.007 &&
      fabs(e.deltaPhiSuperClusterTrackAtVtx()) < 0.15 &&
      e.sigmaIetaIeta() < 0.03 &&
      e.hadronicOverEm() < 0.12  // &&
//      e.dr03EcalRecHitSumEt()/e.pt() < 0.2 &&
//      e.dr03HcalTowerSumEt()/e.pt() < 0.2 &&
//      e.dr03TkSumPt()/e.pt() < 0.2
    );
}

static bool accept_mu(const reco::Muon& m, const reco::Vertex& vtx)
{
    return (
        fabs(m.eta()) < 2.1 &&
        muon::isTightMuon(m, vtx)
    );
}

static bool hasTopMother(const GenParticle * gp, int maxDepth) {
    if (maxDepth < 0 || !gp->numberOfMothers()) {
        return false;
    }
    const GenParticle * mom = (const GenParticle *) gp->mother(0);
    if (abs(mom->pdgId()) == 6) {
        return true;
    }
    return hasTopMother(mom, maxDepth-1);
}

// ------------ method called for each event  ------------
void
TriggerStudies::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
   using namespace edm;
   using namespace std;

   bool changedConfig = false;
   if (!hltConfig_.init(iEvent.getRun(), iSetup, processName_, changedConfig)) {
     cout << "Initialization of HLTConfigProvider failed!!" << endl;
     return;
   }
   if (changedConfig){
     triggerBit_ = -1;
     triggerBitsComb_.clear();
     std::cout << "the curent menu is " << hltConfig_.tableName() << std::endl;
     hltConfig_.dump("Triggers");
     for (size_t j = 0; j < hltConfig_.triggerNames().size(); j++) {
       std::cout << TString(hltConfig_.triggerNames()[j]) << std::endl;
       if (TString(hltConfig_.triggerNames()[j]).Contains(triggerPath_)) {
         triggerBit_ = j;
       }
       for (const auto & trg : triggerPathComb_) {
         if (TString(hltConfig_.triggerNames()[j]).Contains(trg)) {
           triggerBitsComb_.push_back(j);
         }
       }
     }
     if (triggerBit_ == -1) cout << "HLT path not found" << endl;
     assert(triggerBitsComb_.size() == triggerPathComb_.size());  // need all accepted
   }

   // Handle<vector<GenParticle>> genParticles;
   // iEvent.getByLabel(InputTag("genParticles"), genParticles);
   // int searchedPdgId = mode_ ? 11 : 13;
   // bool foundLepton = false;
   // for (const auto & gp : *(genParticles.product())) {
   //   if (abs(gp.pdgId()) == searchedPdgId && hasTopMother(&gp, 10)) {
   //     foundLepton = true;
   //     break;
   //   }
   // }
   // if (!foundLepton) {
   //   return;
   // }

   edm::InputTag triggerResultsLabel("TriggerResults", "", processName_);
   edm::Handle<edm::TriggerResults> triggerResults;
   iEvent.getByLabel(triggerResultsLabel, triggerResults);

   edm::Handle<edm::View<reco::PFJet> > jets_coll;
   iEvent.getByLabel(jets_inp_, jets_coll);

   edm::Handle<edm::View<reco::PFCandidate> > muon_coll;
   iEvent.getByLabel(muon_inp_, muon_coll);

   edm::Handle<edm::View<reco::PFCandidate> > ele_coll;
   iEvent.getByLabel(ele_inp_, ele_coll);

   edm::Handle<edm::View<reco::PFMET> > met_coll;
   iEvent.getByLabel(met_inp_, met_coll);

   edm::Handle<edm::View<reco::Vertex> > vtx_coll;
   iEvent.getByLabel("offlinePrimaryVertices", vtx_coll);

   //pull out pts and calculate ST
   double lepton_pt = 0;
   double lepton_pt_cut = 0;
   double jet_lead_pt = 0;
   double jet_subl_pt = 0;
   double st = 0;
   if (mode_ == 1) {
      lepton_pt_cut = ele_pt_cut_;
      for (auto i = ele_coll->begin(); i!=ele_coll->end(); ++i) {
         if (accept_ele(*(i->gsfElectronRef().get()))) {  // (*i).pt() > ele_pt_cut_ &&
            lepton_pt = i->pt();
            break;
         }
      }
   } else {
      lepton_pt_cut = muon_pt_cut_;
      for (auto i = muon_coll->begin(); i!=muon_coll->end(); ++i) {
         if (accept_mu(*(i->muonRef().get()), *(vtx_coll->begin()))) {  // (*i).pt() > muon_pt_cut_ &&
            lepton_pt = i->pt();
            break;
         }
      }
   }
   for (auto i = jets_coll->begin(); i!=jets_coll->end(); ++i) {
      if (fabs(i->eta()) < jet_eta_) {
         if (jet_lead_pt < 1.) {
            jet_lead_pt = i->pt();
         } else {
            jet_subl_pt = i->pt();
            break;
         }
      }
   }
   for (auto i = jets_coll->begin(); i!=jets_coll->end(); ++i) {
      if (fabs(i->eta()) < jet_eta_ && i->pt() > 40.) {
         st += i->pt();
      }
   }
   st += lepton_pt;
   st += met_coll->at(0).pt();

   // baseline
   histos1D_[ "leptonPt" ]->Fill(lepton_pt);
   histos1D_[ "jetPt"    ]->Fill(jet_lead_pt);
   histos1D_[ "jet2Pt"   ]->Fill(jet_subl_pt);
   histos1D_[ "ST"       ]->Fill(st);

   // check trigger legs
   bool trig_accept = triggerResults->accept(triggerBit_);
   for (const int trg : triggerBitsComb_) {
     trig_accept = trig_accept || triggerResults->accept(trg);
     if (trig_accept) {
       break;
     }
   }
   if (lepton_pt > 1.
       && jet_lead_pt > jet_lead_pt_cut_
       && jet_subl_pt > jet_subl_pt_cut_
   ) {
      histos1D_[ "leptonPtDenom" ]->Fill(lepton_pt);
      if (trig_accept) {
         histos1D_[ "leptonPtPassing" ]->Fill(lepton_pt);
      }
   }
   if (lepton_pt > lepton_pt_cut && jet_subl_pt > jet_subl_pt_cut_) {
      histos1D_[ "jetPtDenom" ]->Fill(jet_lead_pt);
      if (trig_accept) {
         histos1D_[ "jetPtPassing" ]->Fill(jet_lead_pt);
      }
   }
   if (lepton_pt > lepton_pt_cut && jet_lead_pt > jet_lead_pt_cut_) {
      histos1D_[ "jet2PtDenom" ]->Fill(jet_subl_pt);
      if (trig_accept) {
         histos1D_[ "jet2PtPassing" ]->Fill(jet_subl_pt);
      }
   }

   // check st
   double st_lep_pt_cut = (mode_) ? st_ele_pt_ : st_muon_pt_;
   if (lepton_pt > st_lep_pt_cut
       && jet_lead_pt > st_lead_jet_pt_) {
      histos1D_[ "STDenom" ]->Fill(st);
      if (trig_accept) {
         histos1D_[ "STPassing" ]->Fill(st);
      }
   }
}


// ------------ method called once each job just before starting event loop  ------------
void 
TriggerStudies::beginJob()
{
  
  edm::Service< TFileService > fs;

  histos1D_[ "leptonPt"         ] = fs->make< TH1D >( "1leptonPt", ";Lepton p_{T} [GeV];", 250, 1e-20, 500);
  histos1D_[ "leptonPtDenom"    ] = fs->make< TH1D >( "1leptonPtDenom", ";Lepton p_{T} [GeV];", 250, 1e-20, 500);
  histos1D_[ "leptonPtPassing"  ] = fs->make< TH1D >( "1leptonPtPassing", ";Lepton p_{T} [GeV];", 250, 1e-20, 500);

  histos1D_[ "jetPt"            ] = fs->make< TH1D >( "2leadJetPt", ";Leading Jet p_{T} [GeV];", 200, 0., 1000);
  histos1D_[ "jetPtDenom"       ] = fs->make< TH1D >( "2leadJetPtDenom", ";Leading Jet p_{T} [GeV];", 200, 0., 1000);
  histos1D_[ "jetPtPassing"     ] = fs->make< TH1D >( "2leadJetPtPassing", ";Leading Jet p_{T} [GeV];", 200, 0., 1000);

  histos1D_[ "jet2Pt"           ] = fs->make< TH1D >( "3subleadJetPt", ";Subleading Jet p_{T} [GeV];", 200, 0., 1000);
  histos1D_[ "jet2PtDenom"      ] = fs->make< TH1D >( "3subleadJetPtDenom", ";Subleading Jet p_{T} [GeV];", 200, 0., 1000);
  histos1D_[ "jet2PtPassing"    ] = fs->make< TH1D >( "3subleadJetPtPassing", ";Subleading Jet p_{T} [GeV];", 200, 0., 1000);

  histos1D_[ "ST"               ] = fs->make< TH1D >( "4ST", ";Sum of p_{T} of Leading Lepton, MET and all Jets with p_{T}>40GeV [GeV];", 200, 0., 2000);
  histos1D_[ "STDenom"          ] = fs->make< TH1D >( "4STDenom", ";Sum of p_{T} of Leading Lepton, MET and all Jets with p_{T}>40GeV [GeV];", 200, 0., 2000);
  histos1D_[ "STPassing"        ] = fs->make< TH1D >( "4STPassing", ";Sum of p_{T} of Leading Lepton, MET and all Jets with p_{T}>40GeV [GeV];", 200, 0., 2000);
}

// ------------ method called once each job just after ending the event loop  ------------
void 
TriggerStudies::endJob() 
{
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
TriggerStudies::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(TriggerStudies);
