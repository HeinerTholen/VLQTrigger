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
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/JetReco/interface/PFJetCollection.h"
#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenRunInfoProduct.h"

#include "CommonTools/Utils/interface/StringObjectFunction.h"

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
      std::string triggerPath_;
      double jet_lead_pt_;
      double jet_subl_pt_;
      double jet_eta_;
      double muon_pt_cut_;
      double ele_pt_cut_;
      StringObjectFunction<reco::Muon> muon_cut_;
      StringObjectFunction<reco::GsfElectron> ele_cut_;
      int mode_;  // muon: 0, electron: 1

      HLTConfigProvider hltConfig;
      int triggerBit;
};

//
// constructors and destructor
//
TriggerStudies::TriggerStudies(const edm::ParameterSet& iConfig):
   muon_cut_(iConfig.getParameter<std::string>("muon_cut"), false),
   ele_cut_(iConfig.getParameter<std::string>("ele_cut"), false)
{
   triggerPath_ = iConfig.getParameter<std::string> ( "trigger_path" );
   jets_inp_	= iConfig.getParameter<edm::InputTag> ( "jets_inp" );
   muon_inp_	= iConfig.getParameter<edm::InputTag> ( "muon_inp" );
   ele_inp_	    = iConfig.getParameter<edm::InputTag> ( "ele_inp" );
   jet_lead_pt_ = iConfig.getParameter<double> ( "jet_lead_pt" );
   jet_subl_pt_ = iConfig.getParameter<double> ( "jet_subl_pt" );
   jet_eta_     = iConfig.getParameter<double> ( "jet_eta" );
   muon_pt_cut_ = iConfig.getParameter<double> ( "muon_pt_cut" );
   ele_pt_cut_  = iConfig.getParameter<double> ( "ele_pt_cut" );
   mode_        = iConfig.getParameter<int> ( "mode" );
}


TriggerStudies::~TriggerStudies()
{}

//
// member functions
//

// ------------ method called for each event  ------------
void
TriggerStudies::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
   using namespace edm;
   using namespace std;

   bool changedConfig = false;
   if (!hltConfig.init(iEvent.getRun(), iSetup, "HLT", changedConfig)) {
     cout << "Initialization of HLTConfigProvider failed!!" << endl;
     return;
   }
   if (changedConfig){
     std::cout << "the curent menu is " << hltConfig.tableName() << std::endl;
     triggerBit = -1;
     hltConfig.dump("Triggers");
     for (size_t j = 0; j < hltConfig.triggerNames().size(); j++) {
       std::cout << TString(hltConfig.triggerNames()[j]) << std::endl;
       if (TString(hltConfig.triggerNames()[j]).Contains(triggerPath_)) {
         triggerBit = j;
       }
     }
     if (triggerBit == -1) cout << "HLT path not found" << endl;
   }
   
   edm::InputTag triggerResultsLabel = edm::InputTag("TriggerResults", "", "HLT");
   edm::Handle<edm::TriggerResults> triggerResults;
   iEvent.getByLabel(triggerResultsLabel, triggerResults);

   edm::Handle<edm::View<reco::PFJet> > jets_coll;
   iEvent.getByLabel(jets_inp_, jets_coll);

   edm::Handle<edm::View<reco::Muon> > muon_coll;
   iEvent.getByLabel(muon_inp_, muon_coll);

   edm::Handle<edm::View<reco::GsfElectron> > ele_coll;
   iEvent.getByLabel(ele_inp_, ele_coll);

   //pull out pts and calculate HT
   double lepton_pt = 0;
   double lepton_pt_cut = 0;
   double jet_lead_pt = 0;
   double jet_subl_pt = 0;
   double ht = 0;
   if (mode_ == 1) {
      for (auto i = ele_coll->begin(); i!=ele_coll->end(); ++i) {
         if (fabs(ele_cut_(*i)) < 1e-20) {
            lepton_pt = i->pt();
            lepton_pt_cut = ele_pt_cut_;
            break;
         }
      }
   } else {
      for (auto i = muon_coll->begin(); i!=muon_coll->end(); ++i) {
         if (fabs(muon_cut_(*i)) < 1e-20) {
            lepton_pt = i->pt();
            lepton_pt_cut = muon_pt_cut_;
            break;
         }
      }
   }
   for (auto i = jets_coll->begin(); i!=jets_coll->end(); ++i) {
      if (fabs(i->eta()) < jet_eta_) {
         if (!jet_lead_pt_) {
            jet_lead_pt_ = i->pt();
         } else {
            jet_subl_pt_ = i->pt();
            break;
         }
      }
   }
   for (auto i = jets_coll->begin(); i!=jets_coll->end(); ++i) {
      if (fabs(i->eta()) < jet_eta_ && i->pt() > 40.) {
         ht += i->pt();
      }
   }

   // baseline
   histos1D_[ "leptonPt" ]->Fill(lepton_pt);
   histos1D_[ "jetPt"    ]->Fill(jet_lead_pt);
   histos1D_[ "jet2Pt"   ]->Fill(jet_subl_pt);
   histos1D_[ "HT"       ]->Fill(ht);

   // check trigger legs
   bool trig_accept = triggerResults->accept(triggerBit);
   if (jet_lead_pt > jet_lead_pt_ && jet_subl_pt > jet_subl_pt_) {
      histos1D_[ "leptonPtDenom" ]->Fill(lepton_pt);
      if (trig_accept) {
         histos1D_[ "leptonPtPassing" ]->Fill(lepton_pt);
      }
   }
   if (lepton_pt > lepton_pt_cut && jet_subl_pt > jet_subl_pt_) {
      histos1D_[ "jetPtDenom" ]->Fill(jet_lead_pt);
      if (trig_accept) {
         histos1D_[ "jetPtPassing" ]->Fill(jet_lead_pt);
      }
   }
   if (lepton_pt > lepton_pt_cut && jet_lead_pt > jet_lead_pt_) {
      histos1D_[ "jet2PtDenom" ]->Fill(jet_subl_pt);
      if (trig_accept) {
         histos1D_[ "jet2PtPassing" ]->Fill(jet_subl_pt);
      }
   }

   // check ht
   histos1D_[ "HTDenom" ]->Fill(ht);
   if (trig_accept) {
      histos1D_[ "HTPassing" ]->Fill(ht);
   }
}


// ------------ method called once each job just before starting event loop  ------------
void 
TriggerStudies::beginJob()
{
  
  edm::Service< TFileService > fs;

  histos1D_[ "leptonPt"         ] = fs->make< TH1D >( "jetPt", ";Leading Jet Pt [GeV];", 40, 0., 1000);
  histos1D_[ "leptonPtDenom"    ] = fs->make< TH1D >( "jetPtDenom", ";Leading Jet Pt [GeV];", 40, 0., 1000);
  histos1D_[ "leptonPtPassing"  ] = fs->make< TH1D >( "jetPtPassing", ";Leading Jet Pt [GeV];", 40, 0., 1000);

  histos1D_[ "jetPt"            ] = fs->make< TH1D >( "jetPt", ";Leading Jet Pt [GeV];", 40, 0., 1000);
  histos1D_[ "jetPtDenom"       ] = fs->make< TH1D >( "jetPtDenom", ";Leading Jet Pt [GeV];", 40, 0., 1000);
  histos1D_[ "jetPtPassing"     ] = fs->make< TH1D >( "jetPtPassing", ";Leading Jet Pt [GeV];", 40, 0., 1000);

  histos1D_[ "jet2Pt"           ] = fs->make< TH1D >( "jetPt2", ";Subleading Jet Pt [GeV];", 40, 0., 1000);
  histos1D_[ "jet2PtDenom"      ] = fs->make< TH1D >( "jetPt2Denom", ";Subleading Jet Pt [GeV];", 40, 0., 1000);
  histos1D_[ "jet2PtPassing"    ] = fs->make< TH1D >( "jetPt2Passing", ";Subleading Jet Pt [GeV];", 40, 0., 1000);

  histos1D_[ "HT"               ] = fs->make< TH1D >( "HT", ";HT [GeV];", 40, 0., 2000);
  histos1D_[ "HTDenom"          ] = fs->make< TH1D >( "HTDenom", ";HT [GeV];", 40, 0., 2000);
  histos1D_[ "HTPassing"        ] = fs->make< TH1D >( "HTPassing", ";HT [GeV];", 40, 0., 2000);
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
