// $Id: AliJetEmbeddingFromPYTHIATask.cxx $
//
// Jet embedding from PYTHIA task.
//
// Author: S.Aiola, C.Loizides

#include "AliJetEmbeddingFromPYTHIATask.h"

#include <TFile.h>
#include <TMath.h>
#include <TString.h>
#include <TRandom.h>
#include <TParameter.h>
#include <TSystem.h>
#include <TH1F.h>

#include "AliVEvent.h"
#include "AliLog.h"

ClassImp(AliJetEmbeddingFromPYTHIATask)

//________________________________________________________________________
AliJetEmbeddingFromPYTHIATask::AliJetEmbeddingFromPYTHIATask() : 
  AliJetEmbeddingFromAODTask("AliJetEmbeddingFromPYTHIATask"),
  fPYTHIAPath(),
  fPtHardBinScaling(),
  fLHC11hAnchorRun(kTRUE),
  fAnchorRun(-1),
  fCurrentPtHardBin(-1),
  fPtHardBinParam(0),
  fHistPtHardBins(0)
{
  // Default constructor.
  SetSuffix("PYTHIAEmbedding");
  fTotalFiles = 2000;
  fRandomAccess = kTRUE;
  SetMC(kTRUE);
}

//________________________________________________________________________
AliJetEmbeddingFromPYTHIATask::AliJetEmbeddingFromPYTHIATask(const char *name, Bool_t drawqa) : 
  AliJetEmbeddingFromAODTask(name, drawqa),
  fPYTHIAPath("/alice/sim/2012/LHC12a15e"),
  fPtHardBinScaling(),
  fLHC11hAnchorRun(kTRUE),
  fAnchorRun(-1),
  fCurrentPtHardBin(-1),
  fPtHardBinParam(0),
  fHistPtHardBins(0)
{
  // Standard constructor.
  SetSuffix("PYTHIAEmbedding");
  fTotalFiles = 2000;
  fRandomAccess = kTRUE;
  SetMC(kTRUE);
}

//________________________________________________________________________
AliJetEmbeddingFromPYTHIATask::~AliJetEmbeddingFromPYTHIATask()
{
  // Destructor
}

//________________________________________________________________________
void AliJetEmbeddingFromPYTHIATask::UserCreateOutputObjects()
{
  if (!fQAhistos)
    return;

  AliJetModelBaseTask::UserCreateOutputObjects();

  fHistPtHardBins = new TH1F("fHistPtHardBins", "fHistPtHardBins", 11, 0, 11);
  fHistPtHardBins->GetXaxis()->SetTitle("p_{T} hard bin");
  fHistPtHardBins->GetYaxis()->SetTitle("total events");
  fOutput->Add(fHistPtHardBins);

  const Int_t ptHardLo[11] = { 0, 5,11,21,36,57, 84,117,152,191,234};
  const Int_t ptHardHi[11] = { 5,11,21,36,57,84,117,152,191,234,1000000};

  for (Int_t i = 1; i < 12; i++) 
    fHistPtHardBins->GetXaxis()->SetBinLabel(i, Form("%d-%d",ptHardLo[i-1],ptHardHi[i-1]));

  PostData(1, fOutput);
}

//________________________________________________________________________
Bool_t AliJetEmbeddingFromPYTHIATask::ExecOnce() 
{
  if (fPtHardBinScaling.GetSize() > 0) {
    Double_t sum = 0;
    for (Int_t i = 0; i < fPtHardBinScaling.GetSize(); i++) 
      sum += fPtHardBinScaling[i];
    
    if (sum == 0) {
      AliWarning("No hard pt bin scaling!");
      sum = fPtHardBinScaling.GetSize();
    }
    
    for (Int_t i = 0; i < fPtHardBinScaling.GetSize(); i++) 
      fPtHardBinScaling[i] /= sum;
  }

  fPtHardBinParam = static_cast<TParameter<int>*>(InputEvent()->FindListObject("PYTHIAPtHardBin"));
  if (!fPtHardBinParam) {
    fPtHardBinParam = new TParameter<int>("PYTHIAPtHardBin", 0);
    AliDebug(2,"Adding pt hard bin param object to the event list...");
    InputEvent()->AddObject(fPtHardBinParam);
  }

  return AliJetEmbeddingFromAODTask::ExecOnce();
}

//________________________________________________________________________
Bool_t AliJetEmbeddingFromPYTHIATask::GetNextEntry()
{
  Int_t newPtHard = GetRandomPtHardBin();

  new (fPtHardBinParam) TParameter<int>("PYTHIAPtHardBin", newPtHard);

  if (fHistPtHardBins)
    fHistPtHardBins->SetBinContent(newPtHard+1, fHistPtHardBins->GetBinContent(newPtHard+1)+1);

  if (newPtHard != fCurrentPtHardBin) {
    fCurrentPtHardBin = newPtHard;
    if (!OpenNextFile()) return kFALSE;
  }

  return AliJetEmbeddingFromAODTask::GetNextEntry();
}

//________________________________________________________________________
Int_t AliJetEmbeddingFromPYTHIATask::GetRandomPtHardBin() 
{
  static Int_t order[20]={-1};

  if (order[0] == -1)
    TMath::Sort(fPtHardBinScaling.GetSize(), fPtHardBinScaling.GetArray(), order);

  Double_t rnd = gRandom->Rndm();
  Double_t sum = 0;
  Int_t ptHard = -1;
  for (Int_t i = 0; i < fPtHardBinScaling.GetSize(); i++) {
    sum += fPtHardBinScaling[order[i]];
    if (sum >= rnd) {
      ptHard = order[i];
      break;
    }
  }

  return ptHard;
}

//________________________________________________________________________
Bool_t AliJetEmbeddingFromPYTHIATask::UserNotify()
{
  if (!fLHC11hAnchorRun)
    return kTRUE;
  
  Int_t runNumber = InputEvent()->GetRunNumber();

  Int_t semiGoodRunList[28] = {169975, 169981, 170038, 170040, 170083, 170084, 170085, 170088, 
			       170089, 170091, 170152, 170155, 170159, 170163, 170193, 170195, 
			       170203, 170204, 170205, 170228, 170230, 170264, 170268, 170269, 
			       170270, 170306, 170308, 170309};

  fAnchorRun = 169838; // Assume it is a good run

  for (Int_t i = 0; i < 28; i++) {
    if (runNumber == semiGoodRunList[i]) { // If it is semi good, change the anchor run
      fAnchorRun = 170040;
      break;
    }
  }

  return kTRUE;
}

//________________________________________________________________________
TString AliJetEmbeddingFromPYTHIATask::GetNextFileName() 
{
  fCurrentAODFileID = TMath::Nint(gRandom->Rndm()*(fTotalFiles-1))+1;

  TString fileName(fPYTHIAPath);
  if (!fileName.EndsWith("/"))
    fileName += "/";
  
  fileName += fAnchorRun;
  fileName += "/";
  fileName += fCurrentPtHardBin;
  fileName += "/";
  if (fCurrentAODFileID < 10)
    fileName += "00";
  else if (fCurrentAODFileID < 100)
    fileName += "0";
  fileName += fCurrentAODFileID;

  if (!gSystem->AccessPathName(Form("%s/AliAOD.root", fileName.Data())))
    fileName += "/AliAOD.root";
  else if (!gSystem->AccessPathName(Form("%s/aod_archive.zip", fileName.Data())))
    fileName += "/aod_archive.zip#AliAOD.root";
  else
    fileName += "/root_archive.zip#AliAOD.root";

  return fileName;
}
