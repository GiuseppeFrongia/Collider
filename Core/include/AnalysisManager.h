#ifndef ANALYSISMANAGER_H
#define ANALYSISMANAGER_H

#include <string>
#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TEfficiency.h"
#include "TGraphErrors.h"

class AnalysisManager {
public:
    AnalysisManager();
    virtual ~AnalysisManager();
    AnalysisManager(const AnalysisManager&) = delete;
    AnalysisManager& operator=(const AnalysisManager&) = delete;

    bool Init(const std::string& inputFile);
    void Run();

private:
    std::vector<double> FormBinEdges(TH1D* hTrueAll, int minEntriesPerBin);
    
    double FitIterative(TH1D* h, double& error); 

    void PlotResiduals2D(TH2D* histo, const std::string& title, const std::string& filename);
    void PlotResiduals1D(TH1D* histo, const std::string& title, const std::string& filename, bool doFit = true);
    void PlotEfficiency1D(TEfficiency* eff, const std::string& title, const std::string& filename);
    void PlotResolutionGraph(TGraphErrors* graph, const std::string& title, const std::string& filename);

    std::string fInFileName;
    std::string fOutFileName;
    TFile* fInFile;
    TTree* fInTree;
    TFile* fOutFile;

    double fVertexZSigma;
    double fErrZLimit;
    double fMaxZ;
    int fMultMinGlobal;
    int fMultMaxGlobal;
    int fMultMinZoom;
    int fMultMaxZoom;
    int fMinEntriesPerBin;
    bool fPlotFlag;
};

#endif // ANALYSISMANAGER_H
