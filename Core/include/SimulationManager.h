#ifndef SIMULATIONMANAGER_H
#define SIMULATIONMANAGER_H

#include <vector>
#include <string>

class TFile;
class TTree;
class Event;
class Transport;
class RandomManager;
class Cylinder;

template <typename T> class TDistr1D;
using Distr1D    = TDistr1D<double>;
using Distr1Dint = TDistr1D<int>;
class Distr3D;

class SimulationManager {
public:
    SimulationManager();
    ~SimulationManager();
    SimulationManager(const SimulationManager&) = delete;
    SimulationManager& operator=(const SimulationManager&) = delete;

    bool Init(const std::string& configFile);
    void Run();

private:
    RandomManager* fRand;
    std::string fOutFileName;
    TFile* fOutFile;
    TTree* fTree;
    Transport* fTransport;
    Event* fEvent;

    int fNEvents;
    double fPileUpMean;

    std::vector<Cylinder*> fLayers;

    Distr3D* fVertex;
    Distr1D* fImpulseDistr;
    Distr1D* fEta;
    Distr1D* fPhi;
    Distr1Dint* fMult;

    bool LoadGeometry(const std::string& geomFileName, bool enableMS);
    void ClearGeometry();
};

#endif // SIMULATIONMANAGER_H
