#include <iostream>
#include <cmath>
#include <algorithm>

#include "TSystem.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TPaveStats.h"
#include "TF1.h"
#include "TGraphAsymmErrors.h"
#include "TAxis.h"

#include "AnalysisManager.h"
#include "InputManager.h"

AnalysisManager::AnalysisManager() : 
    fInFile(nullptr), fInTree(nullptr), fOutFile(nullptr),
    fVertexZSigma(5.0), fErrZLimit(0.1), fMultMinGlobal(1), fMultMaxGlobal(50),
    fMultMinZoom(5), fMultMaxZoom(20), fMinEntriesPerBin(1000) {}

AnalysisManager::~AnalysisManager() {
    if(fInFile) { fInFile->Close(); delete fInFile; }
    if(fOutFile) { fOutFile->Close(); delete fOutFile; }
}

bool AnalysisManager::Init(const std::string& inputFile) {
    InputManager parser(inputFile);
    if(!parser.IsLoaded()) return false;

    fInFileName  = parser.GetParameter<std::string>("InputFile", "data/2_reconstruction/reco_data.root");
    fOutFileName = parser.GetParameter<std::string>("OutputFile", "data/3_analysis/analysis_data.root");

    fVertexZSigma     = parser.GetParameter<double>("VertexZSigma", 5.0);
    fErrZLimit        = parser.GetParameter<double>("ErrZLimit", 0.05);
    fMaxZ             = parser.GetParameter<double>("VertexZEdges", 15.0);
    fMultMinGlobal    = parser.GetParameter<int>("MultiplicityMin", 1);
    fMultMaxGlobal    = parser.GetParameter<int>("MultiplicityMax", 50);
    fMultMinZoom      = parser.GetParameter<int>("MultMinZoom", 5);
    fMultMaxZoom      = parser.GetParameter<int>("MultMaxZoom", 20);
    fMinEntriesPerBin = parser.GetParameter<int>("MinEntriesPerBin", 1000);
    fPlotFlag         = parser.GetParameter<bool>("PlotFlag", true);
    
    fInFile = TFile::Open(fInFileName.c_str(), "READ");
    if(!fInFile || fInFile->IsZombie()) {
        std::cerr << "[ERROR] AnalysisManager: Impossibile aprire " << fInFileName << std::endl;
        return false;
    }

    fInTree = (TTree*)fInFile->Get("RecoTree");
    if(!fInTree) {
        std::cerr << "[ERROR] RecoTree non trovato!" << std::endl;
        return false;
    }

    gSystem->mkdir("data/3_analysis/plots", true);
    fOutFile = new TFile(fOutFileName.c_str(), "RECREATE");
    fOutFile->cd();

    return true;
}

void AnalysisManager::Run() {
    double zReco = 0.0, zTrue = 0.0;
    int trueMult = 0;
    bool success = false;

    fInTree->SetBranchAddress("Zreco", &zReco);
    fInTree->SetBranchAddress("Ztrue", &zTrue);
    fInTree->SetBranchAddress("TrueMult", &trueMult);
    fInTree->SetBranchAddress("Success", &success);

    int nbinMG = fMultMaxGlobal - fMultMinGlobal + 1;
    double errLimitUm = fErrZLimit * 1e4;

    TH2D* ErrMultHisto2D    = new TH2D("ErrMultHisto",    ";Vera Molteplicita';Z_{rec}-Z_{true} (#mum)", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5, 200, -errLimitUm, errLimitUm);
    TH2D* ErrMultHisto2D_1s = new TH2D("ErrMultHisto_1s", ";Vera Molteplicita';Z_{rec}-Z_{true} (#mum)", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5, 200, -errLimitUm, errLimitUm);
    TH2D* ErrMultHisto2D_3s = new TH2D("ErrMultHisto_3s", ";Vera Molteplicita';Z_{rec}-Z_{true} (#mum)", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5, 200, -errLimitUm, errLimitUm);

    TH1D* MultEventsHisto     = new TH1D("MultEventsHisto",    ";Vera Molteplicita';Eventi", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);
    TH1D* MultSuccessHisto    = new TH1D("MultSuccessHisto",   ";Vera Molteplicita';Eventi", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);
    TH1D* MultEventsHisto_1s  = new TH1D("MultEventsHisto_1s", ";Vera Molteplicita';Eventi", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);
    TH1D* MultSuccessHisto_1s = new TH1D("MultSuccessHisto_1s",";Vera Molteplicita';Eventi", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);
    TH1D* MultEventsHisto_3s  = new TH1D("MultEventsHisto_3s", ";Vera Molteplicita';Eventi", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);
    TH1D* MultSuccessHisto_3s = new TH1D("MultSuccessHisto_3s",";Vera Molteplicita';Eventi", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);

    TH1D* hZTrueAllTmp = new TH1D("hZTrueAllTmp", "", 500, -fMaxZ, fMaxZ);
    TH2D* ErrZHisto2D  = new TH2D("ErrZHisto2D", ";Z_{true} (cm);Z_{rec}-Z_{true} (#mum)", 100, -fMaxZ, fMaxZ, 200, -errLimitUm, errLimitUm);

    // --- EVENT LOOP ---
    int entries = fInTree->GetEntries();
    std::cout << "[INFO] Analizzando " << entries << " eventi..." << std::endl;

    for (int i = 0; i < entries; ++i) {
        fInTree->GetEntry(i);

        hZTrueAllTmp->Fill(zTrue);
        MultEventsHisto->Fill(trueMult);
        
        bool is1Sigma = (std::abs(zTrue) < fVertexZSigma);
        bool is3Sigma = (std::abs(zTrue) < 3*fVertexZSigma);

        if(is1Sigma) MultEventsHisto_1s->Fill(trueMult);
        if(is3Sigma) MultEventsHisto_3s->Fill(trueMult);

        if (success) {
            double residualUm = (zReco - zTrue) * 1e4;

            MultSuccessHisto->Fill(trueMult);
            ErrMultHisto2D->Fill(trueMult, residualUm);
            ErrZHisto2D->Fill(zTrue, residualUm);

            if(is1Sigma) {
                MultSuccessHisto_1s->Fill(trueMult);
                ErrMultHisto2D_1s->Fill(trueMult, residualUm);
            }
            if(is3Sigma) {
                MultSuccessHisto_3s->Fill(trueMult);
                ErrMultHisto2D_3s->Fill(trueMult, residualUm);
            }
        }
    }

    // --- BINNING DINAMICO PER Z ---
    std::vector<double> binEdgesZ = FormBinEdges(hZTrueAllTmp, fMinEntriesPerBin);
    int nBinsZ = binEdgesZ.size() - 1;
    double* binEdgesArray = binEdgesZ.data(); 

    TH1D* ZEventsHisto  = new TH1D("ZEventsHisto",  ";Z_{true} (cm);Eventi", nBinsZ, binEdgesArray);
    TH1D* ZSuccessHisto = new TH1D("ZSuccessHisto", ";Z_{true} (cm);Eventi", nBinsZ, binEdgesArray);

    for (int i = 0; i < entries; ++i) {
        fInTree->GetEntry(i);
        ZEventsHisto->Fill(zTrue);
        if(success) ZSuccessHisto->Fill(zTrue);
    }

    // --- EFFICIENZE ---
    TEfficiency* effMult    = new TEfficiency(*MultSuccessHisto, *MultEventsHisto);
    TEfficiency* effMult_1s = new TEfficiency(*MultSuccessHisto_1s, *MultEventsHisto_1s);
    TEfficiency* effMult_3s = new TEfficiency(*MultSuccessHisto_3s, *MultEventsHisto_3s);
    TEfficiency* effZ       = new TEfficiency(*ZSuccessHisto, *ZEventsHisto);

    effMult->SetName("EffVsMult"); effMult_1s->SetName("EffVsMult_1s"); 
    effMult_3s->SetName("EffVsMult_3s"); effZ->SetName("EffVsZ");

    // --- ESTRAZIONE RISOLUZIONI TRAMITE TGRAPHERRORS ---
    TGraphErrors* grResMult    = new TGraphErrors(); grResMult->SetName("grResMult");
    TGraphErrors* grResMult_1s = new TGraphErrors(); grResMult_1s->SetName("grResMult_1s");
    TGraphErrors* grResMult_3s = new TGraphErrors(); grResMult_3s->SetName("grResMult_3s");
    TGraphErrors* grResZ       = new TGraphErrors(); grResZ->SetName("grResZ");

    for(int i_mult = fMultMinGlobal; i_mult <= fMultMaxGlobal; i_mult++) {
        int targetBin = ErrMultHisto2D->GetXaxis()->FindBin(i_mult);
        double multVal = ErrMultHisto2D->GetXaxis()->GetBinCenter(targetBin);
        double err_sigma = 0;

        // Global
        TH1D* slice = ErrMultHisto2D->ProjectionY("s_glob", targetBin, targetBin);
        if(slice->GetEntries() > 15) {
            double sigma = FitIterative(slice, err_sigma);
            int pt = grResMult->GetN();
            grResMult->SetPoint(pt, multVal, sigma);
            grResMult->SetPointError(pt, 0, err_sigma);
        }
        delete slice;

        // 1 Sigma
        slice = ErrMultHisto2D_1s->ProjectionY("s_1s", targetBin, targetBin);
        if(slice->GetEntries() > 15) {
            double sigma = FitIterative(slice, err_sigma);
            int pt = grResMult_1s->GetN();
            grResMult_1s->SetPoint(pt, multVal, sigma);
            grResMult_1s->SetPointError(pt, 0, err_sigma);
        }
        delete slice;

        // 3 Sigma
        slice = ErrMultHisto2D_3s->ProjectionY("s_3s", targetBin, targetBin);
        if(slice->GetEntries() > 15) {
            double sigma = FitIterative(slice, err_sigma);
            int pt = grResMult_3s->GetN();
            grResMult_3s->SetPoint(pt, multVal, sigma);
            grResMult_3s->SetPointError(pt, 0, err_sigma);
        }
        delete slice;
    }

    // Risoluzione vs Z
    for(int i = 1; i <= nBinsZ; i++) {
        int bin1 = ErrZHisto2D->GetXaxis()->FindBin(binEdgesArray[i-1] + 1e-4);
        int bin2 = ErrZHisto2D->GetXaxis()->FindBin(binEdgesArray[i] - 1e-4);
        
        TH1D* slice = ErrZHisto2D->ProjectionY("s_z", bin1, bin2);
        double err_sigma = 0;
        if(slice->GetEntries() > 15) {
            double sigma = FitIterative(slice, err_sigma);
            int pt = grResZ->GetN();
            double centerZ = (binEdgesArray[i-1] + binEdgesArray[i]) / 2.0;
            double errZ = (binEdgesArray[i] - binEdgesArray[i-1]) / 2.0;
            
            grResZ->SetPoint(pt, centerZ, sigma);
            grResZ->SetPointError(pt, errZ, err_sigma);
        }
        delete slice;
    }

    // --- PLOT E SALVATAGGIO ---
    int bMin = ErrMultHisto2D->GetXaxis()->FindBin(fMultMinZoom);
    int bMax = ErrMultHisto2D->GetXaxis()->FindBin(fMultMaxZoom);
    TH1D* ErrMultHistoFull   = ErrMultHisto2D->ProjectionY("ErrMultHistoFull");
    TH1D* ErrMultHistoSelect = ErrMultHisto2D->ProjectionY("ErrMultHistoSelect", bMin, bMax);

    if(fPlotFlag){
        PlotResiduals1D(ErrMultHistoFull, "Residui Globali Spacchettati", "residuals_full", true);
        PlotResiduals1D(ErrMultHistoSelect, "Residui Selezionati", "residuals_selected", true);

        PlotResiduals2D(ErrMultHisto2D, "Residui vs Molteplicita'", "residuals2D_vs_mult");
        PlotResiduals2D(ErrMultHisto2D_1s, "Residui vs Molteplicita' (|Z_{true}| < #sigma)", "residuals2D_vs_mult_1sigma");
        PlotResiduals2D(ErrMultHisto2D_3s, "Residui vs Molteplicita' (|Z_{true}| < 3#sigma)", "residuals2D_vs_mult_3sigma");
        PlotResiduals2D(ErrZHisto2D, "Residui vs Coordinate Z_{true}", "residuals2D_vs_Zvert");

        PlotResolutionGraph(grResMult, "Risoluzione vs Molteplicita' Globale", "resolution_vs_mult");
        PlotResolutionGraph(grResMult_1s, "Risoluzione vs Molteplicita' (|Z_{true}| < #sigma)", "resolution_vs_mult_1sigma");
        PlotResolutionGraph(grResMult_3s, "Risoluzione vs Molteplicita' (|Z_{true}| < 3#sigma)", "resolution_vs_mult_3sigma");
        PlotResolutionGraph(grResZ, "Risoluzione in funzione di Z_{true}", "resolution_vs_Zvert");

        PlotEfficiency1D(effMult, "Efficienza vs Molteplicita' Globale", "efficiency_vs_mult");
        PlotEfficiency1D(effMult_1s, "Efficienza vs Molteplicita' (|Z_{true}| < #sigma)", "efficiency_vs_mult_1sigma");
        PlotEfficiency1D(effMult_3s, "Efficienza vs Molteplicita' (|Z_{true}| < 3#sigma)", "efficiency_vs_mult_3sigma");
        PlotEfficiency1D(effZ, "Efficienza in funzione della coordinata Z_{true}", "efficiency_vs_Zvert");
    }
    
    fOutFile->cd();
    ErrMultHisto2D->Write(); ErrZHisto2D->Write();
    grResMult->Write(); grResMult_1s->Write(); grResMult_3s->Write(); grResZ->Write();
    effMult->Write(); effMult_1s->Write(); effMult_3s->Write(); effZ->Write();

    delete hZTrueAllTmp;
    delete ErrMultHistoFull; delete ErrMultHistoSelect;
    std::cout << "[INFO] Analisi integrata completata con successo!" << std::endl;
}

double AnalysisManager::FitIterative(TH1D* h, double& error) {
    error = 0.0;
    if (!h || h->GetEntries() < 15) return 0.0;
    
    h->Fit("gaus", "Q0");
    TF1* func = h->GetFunction("gaus");
    if (!func) { error = h->GetRMSError(); return h->GetRMS(); }
    
    double mean = func->GetParameter(1);
    double sigma = func->GetParameter(2);
    
    h->Fit("gaus", "Q0", "", mean - 3*sigma, mean + 3*sigma);
    func = h->GetFunction("gaus");
    if (!func) return sigma;
    mean = func->GetParameter(1);
    sigma = func->GetParameter(2);
    
    h->Fit("gaus", "Q0", "", mean - 1.5*sigma, mean + 1.5*sigma);
    func = h->GetFunction("gaus");
    if (!func) return sigma;

    error = func->GetParError(2);
    return func->GetParameter(2); 
}

std::vector<double> AnalysisManager::FormBinEdges(TH1D* hTrueAll, int minEntriesPerBin) {
    std::vector<double> edges;
    int nBins = hTrueAll->GetNbinsX();
    edges.push_back(hTrueAll->GetXaxis()->GetXmin());
    
    double currentEntries = 0;
    for (int i = 1; i <= nBins; ++i) {
        currentEntries += hTrueAll->GetBinContent(i);
        if (currentEntries >= minEntriesPerBin || i == nBins) {
            edges.push_back(hTrueAll->GetXaxis()->GetBinUpEdge(i));
            currentEntries = 0;
        }
    }
    if (edges.size() > 2 && (edges.back() - edges[edges.size()-2] < 0.5)) {
        edges.erase(edges.end() - 2);
    }
    return edges;
}

void AnalysisManager::PlotResiduals2D(TH2D* histo, const std::string& title, const std::string& filename) {
    TCanvas *c = new TCanvas(title.c_str(), title.c_str(), 1000, 600);
    c->SetGrid();
    histo->SetTitle(title.c_str());
    histo->Draw("COLZ");
    gStyle->SetOptStat("e");
    c->SaveAs(("data/3_analysis/plots/" + filename + ".png").c_str());
    c->SaveAs(("data/3_analysis/plots/" + filename + ".pdf").c_str());
    delete c;
}

void AnalysisManager::PlotResiduals1D(TH1D* histo, const std::string& title, const std::string& filename, bool doFit) {
    TCanvas *c = new TCanvas(title.c_str(), title.c_str(), 1000, 600);
    c->SetGrid();
    histo->SetMarkerColor(kBlue); histo->SetLineColor(kBlue-3);
    histo->SetMarkerStyle(20); histo->SetMarkerSize(0.6);
    histo->SetTitle(title.c_str());
    
    if(doFit) {
        histo->Draw("E1"); histo->Fit("gaus", "Q"); gStyle->SetOptFit(1111);
    } else {
        histo->Draw("P"); gStyle->SetOptStat(0);
    }
    c->SaveAs(("data/3_analysis/plots/" + filename + ".png").c_str());
    c->SaveAs(("data/3_analysis/plots/" + filename + ".pdf").c_str());
    delete c;
}

void AnalysisManager::PlotEfficiency1D(TEfficiency* eff, const std::string& title, const std::string& filename) {
    TCanvas *c = new TCanvas(title.c_str(), title.c_str(), 1000, 600);
    c->SetGrid();
    eff->SetMarkerColor(kRed+1); eff->SetLineColor(kRed+1);
    eff->SetMarkerStyle(20); eff->SetMarkerSize(0.8);
    eff->SetTitle((title + ";;Efficienza").c_str());
    eff->Draw("AP");
    c->Update();
    if(eff->GetPaintedGraph()) eff->GetPaintedGraph()->GetYaxis()->SetRangeUser(0.0, 1.05);
    c->SaveAs(("data/3_analysis/plots/" + filename + ".png").c_str());
    c->SaveAs(("data/3_analysis/plots/" + filename + ".pdf").c_str());
    delete c;
}

void AnalysisManager::PlotResolutionGraph(TGraphErrors* graph, const std::string& title, const std::string& filename) {
    TCanvas *c = new TCanvas(title.c_str(), title.c_str(), 1000, 600);
    c->SetGrid();
    graph->SetMarkerColor(kGreen+3); graph->SetLineColor(kGreen+3);
    graph->SetMarkerStyle(21); graph->SetMarkerSize(0.8);
    
    std::string xTitle = (title.find("Z_{true}") != std::string::npos) ? "Z_{true} (cm)" : "Vera Molteplicita'";
    graph->SetTitle((title + ";" + xTitle + ";Risoluzione (#mum)").c_str());
    
    graph->Draw("AP");
    c->SaveAs(("data/3_analysis/plots/" + filename + ".png").c_str());
    c->SaveAs(("data/3_analysis/plots/" + filename + ".pdf").c_str());
    delete c;
}
