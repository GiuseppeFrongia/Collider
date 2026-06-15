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
#include "ProgressBar.h"
#include "InputManager.h"

AnalysisManager::AnalysisManager() : 
    fInFile(nullptr), fInTree(nullptr), fOutFile(nullptr),
    fVertexZSigma(5.0), fErrZLimit(0.1), fExpectedRes(15.0), fBinsPerSigma(4.0),
    fMultMinGlobal(1), fMultMaxGlobal(50),fMultMinZoom(5), fMultMaxZoom(20),
    fSlicesPerSigmaZ(10.0), fMinEntriesPerBin(1000) {}

AnalysisManager::~AnalysisManager() {
    if(fInFile) { fInFile->Close(); delete fInFile; }
    if(fOutFile) { fOutFile->Close(); delete fOutFile; }
}

bool AnalysisManager::Init(const std::string& inputFile) {
    InputManager parser(inputFile);
    if(!parser.IsLoaded()) return false;

    fInFileName  = parser.GetParameter<std::string>("InputFile", "data/2_reconstruction/reco_data.root");
    fOutFileName = parser.GetParameter<std::string>("OutputFile", "data/3_analysis/analysis_data.root");

    size_t lastSlash = fOutFileName.find_last_of("/\\");
    std::string outDir = (lastSlash != std::string::npos) ? fOutFileName.substr(0, lastSlash) : ".";
    fPlotDir = outDir + "/plots/";

    fMaxZ             = parser.GetParameter<double>("ZLimit", 15.0);
    fVertexZSigma     = parser.GetParameter<double>("VertexZSigma", 5.0);
    fSlicesPerSigmaZ  = parser.GetParameter<double>("SlicesPerSigmaZ", 10.0);
    fErrZLimit        = parser.GetParameter<double>("ResLimit", 0.1);
    fExpectedRes      = parser.GetParameter<double>("ExpectedResol", 0.0015);
    fBinsPerSigma     = parser.GetParameter<double>("BinsPerSigma", 4.0);
    fMultMinGlobal    = parser.GetParameter<int>("MultiplicityMin", 1);
    fMultMaxGlobal    = parser.GetParameter<int>("MultiplicityMax", 50);
    fMultMinZoom      = parser.GetParameter<int>("MultMinZoom", 5);
    fMultMaxZoom      = parser.GetParameter<int>("MultMaxZoom", 20);
    fMinEntriesPerBin = parser.GetParameter<int>("MinEntriesPerBin", 1000);
    fPlotFlag         = parser.GetParameter<bool>("PlotFlag", true);
    fDoErrFit         = parser.GetParameter<bool>("fDoErrFit", true);
    
    fInFile = TFile::Open(fInFileName.c_str(), "READ");
    if(!fInFile || fInFile->IsZombie()) {
        std::cerr << "[ERROR] " << fInFileName << " not found!" << std::endl;
        return false;
    }

    fInTree = (TTree*)fInFile->Get("RecoTree");
    if(!fInTree) {
        std::cerr << "[ERROR] RecoTree not found!" << std::endl;
        return false;
    }

    if(fPlotFlag) gSystem->mkdir(fPlotDir.c_str(), true);
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
    double fExpectedResUm = fExpectedRes * 1e4;

    double binWidthErr = fExpectedRes / fBinsPerSigma;
    double totalErr = 2.0 * errLimitUm;
    int nBinsErr = std::ceil(totalErr / binWidthErr);
    if (nBinsErr < 10) nBinsErr = 10;
    if (nBinsErr > 500) nBinsErr = 500;
    
    double idealSliceZ = fVertexZSigma / fSlicesPerSigmaZ;
    double totalLengthZ = 2.0 * fMaxZ;
    int nBinsZ_2D = std::ceil(totalLengthZ / idealSliceZ);
    if (nBinsZ_2D < 10) nBinsZ_2D = 10;
    if (nBinsZ_2D > 150) nBinsZ_2D = 150;

    TH2D* ErrMultHisto2D    = new TH2D("ErrMultHisto",    ";Vera Molteplicita;Z_{rec}-Z_{true} (#mum)", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5, nBinsErr, -errLimitUm, errLimitUm);
    TH2D* ErrMultHisto2D_1s = new TH2D("ErrMultHisto_1s", ";Vera Molteplicita;Z_{rec}-Z_{true} (#mum)", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5, nBinsErr, -errLimitUm, errLimitUm);
    TH2D* ErrMultHisto2D_3s = new TH2D("ErrMultHisto_3s", ";Vera Molteplicita;Z_{rec}-Z_{true} (#mum)", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5, nBinsErr, -errLimitUm, errLimitUm);

    TH2D* ZMultEventsHisto2D  = new TH2D("ZMultEventsHisto2D", ";Z_{true} (cm);Vera Molteplicita", nBinsZ_2D, -fMaxZ, fMaxZ, nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);
    TH2D* ZMultSuccessHisto2D = new TH2D("ZMultSuccessHisto2D",";Z_{true} (cm);Vera Molteplicita", nBinsZ_2D, -fMaxZ, fMaxZ, nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);
    
    TH1D* MultEventsHisto     = new TH1D("MultEventsHisto",    ";Vera Molteplicita;Eventi", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);
    TH1D* MultSuccessHisto    = new TH1D("MultSuccessHisto",   ";Vera Molteplicita;Eventi", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);
    TH1D* MultEventsHisto_1s  = new TH1D("MultEventsHisto_1s", ";Vera Molteplicita;Eventi", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);
    TH1D* MultSuccessHisto_1s = new TH1D("MultSuccessHisto_1s",";Vera Molteplicita;Eventi", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);
    TH1D* MultEventsHisto_3s  = new TH1D("MultEventsHisto_3s", ";Vera Molteplicita;Eventi", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);
    TH1D* MultSuccessHisto_3s = new TH1D("MultSuccessHisto_3s",";Vera Molteplicita;Eventi", nbinMG, fMultMinGlobal-0.5, fMultMaxGlobal+0.5);
    
    // --- EVENT LOOP ---
    int entries = fInTree->GetEntries();
    std::vector<double> zTrueBuffer;
    zTrueBuffer.reserve(entries);

    ProgressBar pBar(entries, "Analysis");
    pBar.Start();

    for (int i = 0; i < entries; ++i) {
        pBar.Update(i);
        fInTree->GetEntry(i);

        zTrueBuffer.push_back(zTrue);
        MultEventsHisto->Fill(trueMult);
        ZMultEventsHisto2D->Fill(zTrue, trueMult);
        
        bool is1Sigma = (std::abs(zTrue) < fVertexZSigma);
        bool is3Sigma = (std::abs(zTrue) < 3*fVertexZSigma);

        if(is1Sigma) MultEventsHisto_1s->Fill(trueMult);
        if(is3Sigma) MultEventsHisto_3s->Fill(trueMult);

        if (success) {
            double residualUm = (zReco - zTrue) * 1e4;

            MultSuccessHisto->Fill(trueMult);
            ErrMultHisto2D->Fill(trueMult, residualUm);
            ZMultSuccessHisto2D->Fill(zTrue, trueMult);

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
    pBar.Update(entries);

    std::vector<double> binEdgesZ = FormBinEdges(zTrueBuffer, fMinEntriesPerBin);
    int nBinsZ = binEdgesZ.size() - 1;
    double* binEdgesArray = binEdgesZ.data(); 

    zTrueBuffer.clear();
    zTrueBuffer.shrink_to_fit();

    TH2D* ErrZHisto2D   = new TH2D("ErrZHisto2D", ";Z_{true} (cm);Z_{rec}-Z_{true} (#mum)", nBinsZ, -fMaxZ, fMaxZ, nBinsErr, -errLimitUm, errLimitUm);
    TH1D* ZEventsHisto  = new TH1D("ZEventsHisto",  ";Z_{true} (cm);Eventi", nBinsZ, binEdgesArray);
    TH1D* ZSuccessHisto = new TH1D("ZSuccessHisto", ";Z_{true} (cm);Eventi", nBinsZ, binEdgesArray);
    
    for (int i = 0; i < entries; ++i) {
        fInTree->GetEntry(i);
        ZEventsHisto->Fill(zTrue);
        if(success) {
            ZSuccessHisto->Fill(zTrue);

            double residualUm = (zReco - zTrue) * 1e4;
            ErrZHisto2D->Fill(zTrue, residualUm);
        }
    }

    // --- EFFICIENZE ---
    TEfficiency* effMult    = new TEfficiency(*MultSuccessHisto, *MultEventsHisto);       effMult->SetName("EffVsMult"); 
    TEfficiency* effMult_1s = new TEfficiency(*MultSuccessHisto_1s, *MultEventsHisto_1s); effMult_1s->SetName("EffVsMult_1s"); 
    TEfficiency* effMult_3s = new TEfficiency(*MultSuccessHisto_3s, *MultEventsHisto_3s); effMult_3s->SetName("EffVsMult_3s"); 
    TEfficiency* effZ       = new TEfficiency(*ZSuccessHisto, *ZEventsHisto);             effZ->SetName("EffVsZ");
    TEfficiency* effZMult2D = new TEfficiency(*ZMultSuccessHisto2D, *ZMultEventsHisto2D); effZMult2D->SetName("EffVsZMult");    

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
        PlotResiduals1D(ErrMultHistoFull, "Residui", "residuals_full", fDoErrFit);
        PlotResiduals1D(ErrMultHistoSelect, "Residui Selezionati", "residuals_selected", fDoErrFit);

        PlotResiduals2D(ErrMultHisto2D, "Residui vs Molteplicita", "residuals2D_vs_mult");
        PlotResiduals2D(ErrMultHisto2D_1s, "Residui vs Molteplicita (|Z_{true}| < #sigma)", "residuals2D_vs_mult_1sigma");
        PlotResiduals2D(ErrMultHisto2D_3s, "Residui vs Molteplicita (|Z_{true}| < 3#sigma)", "residuals2D_vs_mult_3sigma");
        PlotResiduals2D(ErrZHisto2D, "Residui vs Z_{true}", "residuals2D_vs_Zvert");

        PlotResolutionGraph(grResMult, "Risoluzione vs Molteplicita", "resolution_vs_mult");
        PlotResolutionGraph(grResMult_1s, "Risoluzione vs Molteplicita (|Z_{true}| < #sigma)", "resolution_vs_mult_1sigma");
        PlotResolutionGraph(grResMult_3s, "Risoluzione vs Molteplicita (|Z_{true}| < 3#sigma)", "resolution_vs_mult_3sigma");
        PlotResolutionGraph(grResZ, "Risoluzione vs Z_{true}", "resolution_vs_Zvert");

        PlotEfficiency1D(effMult, "Efficienza vs Molteplicita", "efficiency_vs_mult");
        PlotEfficiency1D(effMult_1s, "Efficienza vs Molteplicita (|Z_{true}| < #sigma)", "efficiency_vs_mult_1sigma");
        PlotEfficiency1D(effMult_3s, "Efficienza vs Molteplicita (|Z_{true}| < 3#sigma)", "efficiency_vs_mult_3sigma");
        PlotEfficiency1D(effZ, "Efficienza vs Z_{true}", "efficiency_vs_Zvert");

        PlotEfficiency2D(effZMult2D, "Z_{true} vs Molteplicita", "efficiency2D_vs_mult");
    }
    
    fOutFile->cd();
    ErrMultHisto2D->Write();
    ErrZHisto2D->Write();

    grResMult->Write();     delete grResMult;
    grResMult_1s->Write();  delete grResMult_1s;
    grResMult_3s->Write();  delete grResMult_3s;
    grResZ->Write();        delete grResZ;
    
    effMult->Write();       delete effMult;
    effMult_1s->Write();    delete effMult_1s;
    effMult_3s->Write();    delete effMult_3s;
    effZ->Write();          delete effZ;
    effZMult2D->Write();    delete effZMult2D;

    delete ErrMultHistoFull; delete ErrMultHistoSelect;
    fOutFile->Close(); delete fOutFile; fOutFile = nullptr;
    std::cout << "[INFO] Analysis completed successfully!" << std::endl;
}

double AnalysisManager::FitIterative(TH1D* h, double& error) {
    error = 0.0;
    if (!h || h->GetEntries() < 15) return 0.0;
    
    // primo check su asse globale
    h->Fit("gaus", "Q0"); 
    TF1* func = h->GetFunction("gaus");
    if (!func) { error = h->GetRMSError(); return h->GetRMS(); }
    
    // primo fit
    double peakMean = func->GetParameter(1);
    double approxRMS = h->GetRMS(); 
    h->Fit("gaus", "Q0", "", peakMean - 2.0*approxRMS, peakMean + 2.0*approxRMS);
    
    // secondo check
    func = h->GetFunction("gaus");
    if (!func) { error = h->GetRMSError(); return h->GetRMS(); }
    
    // secondo fit
    double mean = func->GetParameter(1);
    double sigma = func->GetParameter(2);
    h->Fit("gaus", "Q0", "", mean - 3.0*sigma, mean + 3.0*sigma);

    // terzo check
    func = h->GetFunction("gaus");
    if (!func) return sigma;

    // terzo fit
    mean = func->GetParameter(1);
    sigma = func->GetParameter(2);
    h->Fit("gaus", "Q0", "", mean - 1.5*sigma, mean + 1.5*sigma);

    // ultimo check
    func = h->GetFunction("gaus");
    if (!func) return sigma;

    error = func->GetParError(2);
    return func->GetParameter(2); 
}

std::vector<double> AnalysisManager::FormBinEdges(std::vector<double>& z_positions, int minEntriesPerBin) {
    std::vector<double> edges;
    int totalEvents = z_positions.size();
    
    if (totalEvents < minEntriesPerBin) {
        edges.push_back(-fMaxZ);
        edges.push_back(fMaxZ);
        return edges;
    }

    std::sort(z_positions.begin(), z_positions.end());

    int theoreticalMaxBins = totalEvents / minEntriesPerBin;
    edges.reserve(theoreticalMaxBins + 2);

    edges.push_back(-fMaxZ);

    for (int i = minEntriesPerBin; i < totalEvents; i += minEntriesPerBin) {
        
        int remainingEvents = totalEvents - i;
        if (remainingEvents < minEntriesPerBin) { break; }
        
        double binBoundary = (z_positions[i - 1] + z_positions[i]) / 2.0;
        if (binBoundary > edges.back() && binBoundary < fMaxZ) {
            edges.push_back(binBoundary);
        }
    }

    edges.push_back(fMaxZ);

    return edges;
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
    c->SaveAs((fPlotDir + filename + ".png").c_str());
    c->SaveAs((fPlotDir + filename + ".pdf").c_str());
    delete c;
}

void AnalysisManager::PlotResiduals2D(TH2D* histo, const std::string& title, const std::string& filename) {
    TCanvas *c = new TCanvas(title.c_str(), title.c_str(), 1000, 600);
    c->SetGrid();

    histo->SetTitle(title.c_str());

    histo->Draw("COLZ");
    gStyle->SetOptStat("e");
    c->Update();
    c->SaveAs((fPlotDir + filename + ".png").c_str());
    c->SaveAs((fPlotDir + filename + ".pdf").c_str());
    delete c;
}

void AnalysisManager::PlotEfficiency1D(TEfficiency* eff, const std::string& title, const std::string& filename) {
    TCanvas *c = new TCanvas(title.c_str(), title.c_str(), 1000, 600);
    c->SetGrid();
    eff->SetMarkerColor(kRed+1); eff->SetLineColor(kRed+1);
    eff->SetMarkerStyle(20); eff->SetMarkerSize(0.8);

    std::string xTitle = (title.find("Molteplicita") != std::string::npos) ? "Vera Molteplicita" : "Z_{true} (cm)";
    eff->SetTitle((title + ";" + xTitle + ";Efficienza").c_str());

    eff->Draw("AP");
    c->Update();
    if(eff->GetPaintedGraph()) {
        eff->GetPaintedGraph()->SetMaximum(1.001);
        
        if (title.find("Molteplicita") != std::string::npos) {
            eff->GetPaintedGraph()->GetXaxis()->SetLimits(fMultMinGlobal - 0.5, fMultMaxGlobal + 0.5);
        } else {
            eff->GetPaintedGraph()->GetXaxis()->SetLimits(-fMaxZ, fMaxZ);
        }
    }
    c->SaveAs((fPlotDir + filename + ".png").c_str());
    c->SaveAs((fPlotDir + filename + ".pdf").c_str());
    delete c;
}

void AnalysisManager::PlotResolutionGraph(TGraphErrors* graph, const std::string& title, const std::string& filename) {
    TCanvas *c = new TCanvas(title.c_str(), title.c_str(), 1000, 600);
    c->SetGrid();
    graph->SetMarkerColor(kGreen+3); graph->SetLineColor(kGreen+3);
    graph->SetMarkerStyle(21); graph->SetMarkerSize(0.8);
    
    std::string xTitle = (title.find("Molteplicita") != std::string::npos) ? "Vera Molteplicita" : "Z_{true} (cm)";
    graph->SetTitle((title + ";" + xTitle + ";Risoluzione (#mum)").c_str());

    graph->Draw("AP");
    c->Update();
    if (title.find("Molteplicita") != std::string::npos) {
            graph->GetXaxis()->SetLimits(fMultMinGlobal - 0.5, fMultMaxGlobal + 0.5);
        } else {
            graph->GetXaxis()->SetLimits(-fMaxZ, fMaxZ);
    }
    
    c->SaveAs((fPlotDir + filename + ".png").c_str());
    c->SaveAs((fPlotDir + filename + ".pdf").c_str());
    delete c;
}

void AnalysisManager::PlotEfficiency2D(TEfficiency* eff, const std::string& title, const std::string& filename) {
    TCanvas *c = new TCanvas(title.c_str(), title.c_str(), 1000, 600);
    c->SetGrid();

    eff->SetTitle((title + ";Z_{true} (cm);Vera Molteplicita;Efficienza").c_str());

    eff->Draw("COLZ");
    //gStyle->SetOptStat("e");
    c->Update();
    c->SaveAs((fPlotDir + filename + ".png").c_str());
    c->SaveAs((fPlotDir + filename + ".pdf").c_str());
    delete c;
}