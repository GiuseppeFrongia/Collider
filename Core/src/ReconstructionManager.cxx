#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

#include "TVector2.h"
#include "TSystem.h"

#include "ReconstructionManager.h"
#include "InputManager.h"
#include "RandomManager.h"
#include "ProgressBar.h"

ReconstructionManager::ReconstructionManager() : 
fInFile(nullptr), 
fInTree(nullptr), 
fEvent(nullptr), 
fOutFile(nullptr),
fOutTree(nullptr),
fDeltaPhiMax(0.0035),
fZWindowSize(0.2),
fAlgorithm("SlidingWindow"),
fSeed(42),
fReSmear(false),
fNewResZ(0.0),
fNewResPhi(0.0),
fAddNoise(false),
fNoiseMean(0.0),
fZReco(std::nan("")),
fZTrue(std::nan("")),
fTrueMult(0),
fSuccess(false) {
}

ReconstructionManager::~ReconstructionManager() { 
    if(fInFile)  { delete fInFile;  fInFile = nullptr; } 
    if(fOutFile) { delete fOutFile; fOutFile = nullptr; } 
}

bool ReconstructionManager::LoadGeometry(const std::string& geomFileName) {
    std::ifstream file(geomFileName.c_str());
    if(!file.is_open()) return false;
    
    struct TempLayer { std::string type; double r, l; };
    std::vector<TempLayer> tempLayers;
    
    std::string line;
    while(std::getline(file, line)){
        if(line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string type;
        double r, l, t;

        ss >> type >> r >> l;
        
        tempLayers.push_back({type, r, l});
    }
    
    std::sort(tempLayers.begin(), tempLayers.end(), [](const TempLayer& a, const TempLayer& b){ return a.r < b.r; });
    
    fLayerGeom.clear();
    
    for(size_t i = 0; i < tempLayers.size(); ++i) {
        if(tempLayers[i].type == "DETECT") {
            fLayerGeom[i] = {tempLayers[i].r, tempLayers[i].l};
        }
    }
    
    return true;
}

bool ReconstructionManager::Init(const std::string& configFile) {
    InputManager parser(configFile); 
    if(!parser.IsLoaded()) return false;
    
    fInFileName  = parser.GetParameter<std::string>("InputFile", "data/1_simulation/sim_data.root"); 
    fOutFileName = parser.GetParameter<std::string>("OutputFile", "data/2_reconstruction/reco_data.root");
    fDeltaPhiMax = parser.GetParameter<double>("DeltaPhiMax", 0.0035); 
    fZWindowSize = parser.GetParameter<double>("ZWindowSize", 0.2);
    fAlgorithm   = parser.GetParameter<std::string>("Algorithm", "SlidingWindow");
    fSeed        = parser.GetParameter<unsigned int>("Seed", 42);

    fReSmear   = parser.GetParameter<bool>("ReSmear", false);
    fNewResZ   = parser.GetParameter<double>("NewResZ", 0.0);
    fNewResPhi = parser.GetParameter<double>("NewResPhi", 0.0);
    
    fAddNoise  = parser.GetParameter<bool>("AddNoise", false);
    fNoiseMean = parser.GetParameter<double>("NoiseMean", 0.0);
    
    std::string geomFile = parser.GetParameter<std::string>("GeometryFile", "input/geometry.txt");
    if(!LoadGeometry(geomFile)) {
        std::cerr << "[ERROR] ReconstructionManager failed to parse geometry file: " << geomFile << std::endl;
        return false;
    }

    fInFile = TFile::Open(fInFileName.c_str(), "READ"); 
    if(!fInFile || fInFile->IsZombie()) {
        std::cerr << "[ERROR] Failed to open simulation input file: " << fInFileName << std::endl;
        return false;
    }
    
    fInTree = (TTree*)fInFile->Get("SimTree"); 
    if(!fInTree) {
        std::cerr << "[ERROR] SimTree not found!" << std::endl;
        fInFile->Close(); delete fInFile; fInFile = nullptr;
        return false;
    }
    
    fInTree->SetBranchAddress("Event", &fEvent);
    
    gSystem->mkdir("data/2_reconstruction", true);
    fOutFile = new TFile(fOutFileName.c_str(), "RECREATE");
    fOutTree = new TTree("RecoTree", "Reconstruction");
    fOutTree->Branch("Zreco", &fZReco); 
    fOutTree->Branch("Ztrue", &fZTrue);
    fOutTree->Branch("TrueMult", &fTrueMult);
    fOutTree->Branch("Success", &fSuccess);
    fOutTree->Branch("nHitsPerLayer", &fNHitsPerLayer);

    return true;
}

void ReconstructionManager::Run() {
    int entries = fInTree->GetEntries();
    
    auto rand = RandomManager::Instance(fSeed);

    ProgressBar pBar(entries, "Reconstruction");
    pBar.Start();

    for(int i = 0; i < entries; ++i){
        pBar.Update(i);  // if (i % 1000 == 0) 
        fInTree->GetEntry(i);
        
        fZTrue = fEvent->GetPrimaryVertex().z;
        fTrueMult = fEvent->GetPrimaryVertex().mult;
        
        std::map<int, std::vector<RecoHit>> hitsByLayer;
        int totHits = fEvent->GetNHits();
        
        for(int j = 0; j < totHits; ++j){
            const Hit* h = fEvent->GetHit(j);
            if(!h->IsDetected()) continue;

            int layerID = h->GetLayerID();
            if(fLayerGeom.find(layerID) == fLayerGeom.end()) continue;
            double radius = fLayerGeom[layerID].radius;
            
            double z = h->GetMeasZ();
            double phi = TVector2::Phi_mpi_pi(h->GetMeasPhi());
            
            if(fReSmear) {
                z = h->GetTrueZ() + rand->Gaus(0.0, fNewResZ);
                phi = h->GetTruePhi() + (rand->Gaus(0.0, fNewResPhi) / radius);
                phi = TVector2::Phi_mpi_pi(phi);
            }
            
            hitsByLayer[h->GetLayerID()].push_back({z, phi, h->GetTrueR()});
        }
        
        if(fAddNoise && (fNoiseMean > 0.0)) {
            for(const auto& geomPair : fLayerGeom) {
                int layerID = geomPair.first;
                auto geom   = geomPair.second;
                
                int nNoiseHits = rand->Poisson(fNoiseMean);
                
                for(int n = 0; n < nNoiseHits; ++n) {
                    double noise_z   = rand->Uniform(-geom.length / 2.0, geom.length / 2.0);
                    double noise_phi = rand->Uniform(-M_PI, M_PI);
                    
                    hitsByLayer[layerID].push_back({noise_z, noise_phi, geom.radius});
                }
            }
        }
        
        fNHitsPerLayer.clear();
        if(!hitsByLayer.empty()) {
            int maxLayer = hitsByLayer.rbegin()->first; 
            fNHitsPerLayer.assign(maxLayer + 1, 0);
            for(const auto& pair : hitsByLayer) {
                fNHitsPerLayer[pair.first] = pair.second.size();
            }
        }

        for(auto& pair : hitsByLayer) {
            std::sort(pair.second.begin(), pair.second.end(), [](const RecoHit& a, const RecoHit& b) {
                return a.phi < b.phi;
            });
        }
        
        std::vector<double> zCandidates;
        
        for(auto it1 = hitsByLayer.begin(); it1 != hitsByLayer.end(); ++it1) {
            for(auto it2 = std::next(it1); it2 != hitsByLayer.end(); ++it2) {
                
                const auto& innerHits = it1->second;
                const auto& outerHits = it2->second;
                
                for(const auto& h1 : innerHits){
                    
                    double phi_min = h1.phi - fDeltaPhiMax;
                    double phi_max = h1.phi + fDeltaPhiMax;
                    
                    // ricerca binaria
                    auto findAndAdd = [&](double min_val, double max_val) {
                        
                        // cerca la prima hit >= min_val (O(log N))
                        auto start_it = std::lower_bound(outerHits.begin(), outerHits.end(), min_val, 
                            [](const RecoHit& h, double val) { return h.phi < val; });
                        
                        // Cerca la prima hit > max_val
                        auto end_it = std::upper_bound(start_it, outerHits.end(), max_val, 
                            [](double val, const RecoHit& h) { return val < h.phi; });
                            
                        // calcola Z0 solo per le hit che sono cadute in questo range
                        for(auto it = start_it; it != end_it; ++it) {
                            double dZ = it->z - h1.z;
                            double dR = it->r - h1.r;

                            if (std::abs(dR) < 1e-6) continue;
                            zCandidates.push_back(h1.z - h1.r * (dZ / dR));
                        }
                    };
                    
                    // gestione geometrica del "Wrap-Around"
                    if (phi_min < -M_PI) {
                        findAndAdd(phi_min + 2.0 * M_PI, M_PI); 
                        findAndAdd(-M_PI, phi_max);             
                    } else if (phi_max > M_PI) {
                        findAndAdd(phi_min, M_PI);              
                        findAndAdd(-M_PI, phi_max - 2.0 * M_PI);
                    } else {
                        findAndAdd(phi_min, phi_max);           
                    }
                }
            }
        }
        
        if(!zCandidates.empty()){
            fZReco = (fAlgorithm == "SlidingWindow") ? SlidingWindow(zCandidates) : HalfSampleMode(zCandidates);
            fSuccess = true;
        } else {
            fZReco = std::nan("");
            fSuccess = false;
        }
        
        fOutTree->Fill();
    }

    pBar.Update(entries);
    
    fOutFile->cd(); 
    fOutTree->Write("", TObject::kOverwrite); 
    delete fOutFile; fOutFile = nullptr; fOutTree = nullptr;
    delete fInFile;  fInFile = nullptr;  fInTree = nullptr;
}

double ReconstructionManager::SlidingWindow(const std::vector<double>& zCands) {
    std::vector<double> sorted = zCands; 
    std::sort(sorted.begin(), sorted.end());
    
    int maxCount = 0; 
    double bestZ = 0.0;
    
    for(size_t i = 0; i < sorted.size(); ++i){
        size_t j = i;
        while(j < sorted.size() && (sorted[j] - sorted[i]) <= fZWindowSize) j++;
        
        int count = j - i;
        if(count > maxCount){
            maxCount = count; 
            double sum = 0;
            for(size_t k = i; k < j; ++k) sum += sorted[k];
            bestZ = sum / count;
        }
    }
    return bestZ;
}

double ReconstructionManager::HalfSampleMode(std::vector<double> zCands) {
    if(zCands.empty()) return 0.0;
    
    std::sort(zCands.begin(), zCands.end());
    
    while(zCands.size() > 3){
        int n = zCands.size();
        int half = (n + 1) / 2;
        double minWidth = 1e9; 
        int bestIdx = 0;
        
        for(int i = 0; i <= n - half; ++i){
            double w = zCands[i + half - 1] - zCands[i];
            if(w < minWidth) { 
                minWidth = w; 
                bestIdx = i; 
            }
        }
        zCands = std::vector<double>(zCands.begin() + bestIdx, zCands.begin() + bestIdx + half);
    }
    
    double sum = 0; 
    for(double z : zCands) sum += z;
    return sum / zCands.size();
}
