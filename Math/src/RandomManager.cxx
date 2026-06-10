#include <cmath>

#include "TFile.h"
#include "TFormula.h"

#include "RandomManager.h"
#include "Distributions.h"


RandomManager* RandomManager::fInstance = nullptr;

RandomManager* RandomManager::Instance(unsigned int seed) {
    if (!fInstance) fInstance = new RandomManager(seed);
    return fInstance;
}

void RandomManager::Destroy() {
    if (fInstance) { delete fInstance; fInstance = nullptr; }
}

RandomManager::RandomManager(unsigned int seed) {
    fRandom = new TRandom3(seed);
    gRandom = fRandom;
}

RandomManager::~RandomManager() {
    if (gRandom == fRandom) gRandom = nullptr;
    delete fRandom;
    fRandom = nullptr;

    for (auto& pair : fHistograms) {
        if (pair.second) delete pair.second; 
    }
    fHistograms.clear();
}

double RandomManager::EvaluateMath(const std::string& expr) {
    TFormula formula("math_eval", expr.c_str());
    if (formula.IsValid()) return formula.Eval(0);
    
    std::cerr << "[WARNING] Failed to evaluate: " << expr << std::endl;
    return 0.0;
}

TH1* RandomManager::GetStoredHistogram(const std::string& fileName, const std::string& histName) {
    std::string key = fileName + ":" + histName;

    auto it = fHistograms.find(key);
    if (it != fHistograms.end()) {
        return it->second;
    }

    TFile* file = TFile::Open(fileName.c_str(), "READ");
    if (!file || file->IsZombie()) {
        std::cerr << "[ERROR] Cannot open ROOT file: " << fileName << std::endl;
        return nullptr;
    }

    TH1* hist = (TH1*)file->Get(histName.c_str());
    if (!hist) {
        std::cerr << "[ERROR] Histogram " << histName << " not found!" << std::endl;
        file->Close(); delete file;
        return nullptr;
    }

    hist->SetDirectory(nullptr); 
    file->Close(); delete file;

    fHistograms[key] = hist;
    return hist;
}

Distr3D* RandomManager::Fact3DJointDistr(const std::string& configStr) {
    std::stringstream sStream(configStr);

    Distr1D* genX = SetDistr(sStream);
    Distr1D* genY = SetDistr(sStream);
    Distr1D* genZ = SetDistr(sStream);

    if (!genX || !genY || !genZ) {
        std::cerr << "[WARNING] RandomManager::Fact3DJointDistr - "
                  << "Invalid string format for a factorizable 3D joint distribution: \"" 
                  << configStr << "\". Returning nullptr." << std::endl;
        delete genX; delete genY; delete genZ;
        return nullptr;
    }

    return new JointFact3D(genX, genY, genZ);
}

Distr1D* RandomManager::SetDistr(const std::string& configStr) {
    std::stringstream sStream(configStr);
    return SetDistr(sStream);
}

Distr1D* RandomManager::SetDistr(std::stringstream& sStream) {
    std::string type;
    sStream >> type;

    if (type == "Gaussian") {
        std::string sMean, sSigma;
        sStream >> sMean >> sSigma;
        return new GaussianDistr(this, EvaluateMath(sMean), EvaluateMath(sSigma)); 
        
    } else if (type == "Uniform") {
        std::string sMin, sMax;
        sStream >> sMin >> sMax;
        return new UniformDistr(this, EvaluateMath(sMin), EvaluateMath(sMax));
        
    } else if (type == "Fixed") {
        std::string sValue;
        sStream >> sValue;
        return new FixedDistr(EvaluateMath(sValue));
        
    } else if (type == "Hist") {
        std::string fileName, histName;
        sStream >> fileName >> histName;
        
        TH1* hist = GetStoredHistogram(fileName, histName);
        if (hist) return new HistDistr(hist);
    }
    
    std::cerr << "[WARNING] Unknown distribution 1D: " << type << std::endl;
    return nullptr;
}

Distr1Dint* RandomManager::SetDistrInt(const std::string& configStr) {
    std::stringstream sStream(configStr);
    std::string type;
    sStream >> type;

    if (type == "Poisson") {
        std::string sMean;
        sStream >> sMean;
        return new PoissonDistr(this, EvaluateMath(sMean));

    } else if (type == "Uniform") {
        std::string sMin, sMax;
        sStream >> sMin >> sMax;
        return new UniformDistrInt(this, EvaluateMath(sMin), EvaluateMath(sMax));

    } else if (type == "Fixed") {
        std::string sValue;
        sStream >> sValue;
        return new FixedDistrInt(EvaluateMath(sValue));

    } else if (type == "Hist") {
        std::string fileName, histName;
        sStream >> fileName >> histName;
        
        TH1* hist = GetStoredHistogram(fileName, histName);
        if (hist) return new HistDistrInt(hist);
    }
    
    std::cerr << "[WARNING] RandomManager::Fact3DJointDistr - "
              <<  "Unknown distribution Int: "
              << type << ". Returning nullptr." << std::endl;
    return nullptr;
}

double RandomManager::Gaus(double mean, double sigma) { return fRandom->Gaus(mean, sigma); }
double RandomManager::Uniform(double min, double max) { return fRandom->Uniform(min, max); }
double RandomManager::Fixed(double fixedValue) { return fixedValue; }
int RandomManager::Poisson(double mean) { return fRandom->Poisson(mean); }

double RandomManager::Module2Pi(double angle) {
    while (angle < 0.0) angle += 2.0 * M_PI;
    while (angle >= 2.0 * M_PI) angle -= 2.0 * M_PI;
    return angle;
}