#ifndef RECONSTRUCTIONMANAGER_H
#define RECONSTRUCTIONMANAGER_H

#include <string>
#include <vector>
#include <map>

#include "TFile.h"
#include "TTree.h"
#include "Event.h"

class ReconstructionManager {
public:
    enum RecoMethod {
        kSlidingWindow,
        kHalfSample
    };

    ReconstructionManager();
    virtual ~ReconstructionManager();

    bool Init(const std::string& configFile);
    void Run();

private:
    struct RecoHit {
        double z;
        double phi;
        double r;
    };

    struct LayerSpecs {
        double radius;
        double length;
    };

    bool LoadGeometry(const std::string& geomFileName);

    std::string fInFileName;
    std::string fOutFileName;

    TFile* fInFile;
    TTree* fInTree;
    Event* fEvent;
    TFile* fOutFile;
    TTree* fOutTree;

    double fDeltaPhiMax;
    double fZWindowSize;
    std::string fAlgorithm;
    unsigned int fSeed;

    bool fReSmear;
    double fNewResZ;
    double fNewResPhi;
    
    bool fAddNoise;
    double fNoiseMean;

    std::map<int, LayerSpecs> fLayerGeom;

    double fZReco; 
    double fZTrue;
    int fTrueMult;
    bool fSuccess;
    std::vector<int> fNHitsPerLayer;

    double SlidingWindow(const std::vector<double>& zCands);
    double HalfSampleMode(std::vector<double> zCands);
};

#endif // RECONSTRUCTIONMANAGER_H