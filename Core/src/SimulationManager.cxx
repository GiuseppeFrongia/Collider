#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

#include "TSystem.h"
#include "TFile.h"
#include "TTree.h"

#include "SimulationManager.h"
#include "InputManager.h"
#include "RandomManager.h"
#include "Cylinder.h"
#include "Event.h"
#include "Particle.h"
#include "Transport.h"
#include "Distributions.h"
#include "ProgressBar.h"

SimulationManager::SimulationManager() :
fRand(nullptr),
fOutFileName(""),
fOutFile(nullptr),
fTree(nullptr),
fEvent(new Event()),
fTransport(nullptr),
fNEvents(1000),
fPileUpMean(1.0),
fVertex(nullptr),
fImpulseDistr(nullptr),
fEta(nullptr),
fPhi(nullptr),
fMult(nullptr) {
}

SimulationManager::~SimulationManager() {
    delete fEvent;
    fEvent = nullptr;
    ClearGeometry();
    
    delete fVertex;
    delete fImpulseDistr;
    delete fEta;
    delete fMult;
    delete fTransport;

    if(fOutFile) { delete fOutFile; fOutFile = nullptr; fTree = nullptr; }
}

void SimulationManager::ClearGeometry() {
    for(auto layer : fLayers) {
        if (layer) delete layer;
    }
    fLayers.clear();
}

bool SimulationManager::LoadGeometry(const std::string& geomFileName, bool enableMS) {
    ClearGeometry();
    std::ifstream file(geomFileName.c_str());
    if(!file.is_open()) {
        std::cerr << "[ERROR] Failed to open geometry file: " << geomFileName << std::endl;
        return false;
    }
    
    std::string line;
    while(std::getline(file, line)){
        if(line.empty() || line[0] == '#') continue;
        
        std::stringstream ss(line);
        std::string type;
        ss >> type; 

        Cylinder* newGeom = Cylinder::Create(type);

        if (newGeom) {
            newGeom->Load(ss);
            fLayers.push_back(newGeom);
        } else {
            std::cerr << "[ERROR] Unknown geometry type: " << type << std::endl;
            return false;
        }
    }

    std::sort(fLayers.begin(), fLayers.end(), [](Cylinder* a, Cylinder* b) {
        return a->GetRadius() < b->GetRadius();
    });

    for (size_t i = 0; i < fLayers.size(); ++i) {
        fLayers[i]->SetLayerID(i);
        fLayers[i]->SetMSswitch(enableMS);}

    return true;
}

bool SimulationManager::Init(const std::string& inputFile) {
    InputManager parser(inputFile);
    if(!parser.IsLoaded()) return false;
    
    unsigned int seed = parser.GetParameter<unsigned int>("Seed", 42);    
    fRand = RandomManager::Instance(seed);

    fNEvents = parser.GetParameter<int>("NEvents", 1000);
    fPileUpMean = parser.GetParameter<double>("PileUpMean", 1.0);

    fVertex       = fRand->Fact3DJointDistr(parser.GetRawString("VertexDistr", "Gaussian 0 5.3 Gaussian 0 0.01 Gaussian 0 0.01"));
    fImpulseDistr = fRand->SetDistr(parser.GetRawString("ImpulseDistr", "Fixed 1.0"));
    fEta          = fRand->SetDistr(parser.GetRawString("EtaDistr", "Uniform -2.0 2.0"));
    fPhi          = fRand->SetDistr(parser.GetRawString("PhiDistr", "Uniform 0 2*pi"));
    fMult         = fRand->SetDistrInt(parser.GetRawString("MultDistr", "Poisson 20"));
    
    fOutFileName = parser.GetParameter<std::string>("OutputFile", "data/1_simulation/sim_data.root");

    fTransport = Transport::Create(parser.GetRawString("Transport", "LinearTransport"));
    if(!fTransport) {
        std::cerr << "[ERROR] Failed to create Transport engine!" << std::endl;
        return false;
    }

    bool enableMS = parser.GetParameter<bool>("EnableMS", true);

    if(!LoadGeometry(parser.GetParameter<std::string>("GeometryFile", "input/geometry.txt"), enableMS)) {
        std::cerr << "[ERROR] Failed to properly load geometry." << std::endl;
        return false;
    }
    
    gSystem->mkdir("data/1_simulation", true);
    fOutFile = new TFile(fOutFileName.c_str(), "RECREATE");
    fTree = new TTree("SimTree", "Simulation MC");
    fTree->Branch("Event", &fEvent);
    return true;
}

void SimulationManager::Run() {
    if(!fTree || !fRand || !fVertex || !fImpulseDistr || !fEta || !fMult) return;
    
    int trackID = 0;
    double xVertex, yVertex, zVertex;

    ProgressBar pBar(fNEvents, "Simulation MC");
    pBar.Start();

    for(int i = 0; i < fNEvents; ++i){
        pBar.Update(i);
        fEvent->Clear();
        
        int nVertices = fRand->Poisson(fPileUpMean) + 1; 
        for(int v = 0; v < nVertices; ++v){
            int mult = fMult->Sample();
            
            fVertex->Sample(xVertex, yVertex, zVertex);
            fEvent->AddVertex(xVertex, yVertex, zVertex, mult);
            
            for(int p = 0; p < mult; ++p){
                double eta = fEta->Sample();
                double theta = 2.0 * std::atan(std::exp(-eta));
                double phi = fPhi->Sample();
                double momentum = fImpulseDistr->Sample();

                double ux = std::sin(theta) * std::cos(phi);
                double uy = std::sin(theta) * std::sin(phi);
                double uz = std::cos(theta);

                Particle part(trackID, xVertex, yVertex, zVertex, ux, uy, uz, momentum);
                int layerID = 0;
                
                for(auto layer : fLayers){
                    
                    if(fTransport->Propagate(part, layer)){
                        
                        Hit hitBuffer;
                        if(layer->Interact(part, hitBuffer)) {
                            fEvent->AddHit(hitBuffer);
                        }
                    } else { break; }
                    layerID++;
                }
                trackID++;
            }
        }
        
        fTree->Fill();
    }
    
    pBar.Update(fNEvents);

    fOutFile->cd(); 
    fTree->Write("", TObject::kOverwrite); //fTree->Write();
    fOutFile->Close(); 
    delete fOutFile; 
    fOutFile = nullptr; 
    fTree = nullptr;
}
