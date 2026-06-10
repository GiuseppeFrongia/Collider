#ifndef RANDOMMANAGER_H
#define RANDOMMANAGER_H

#include <string>
#include <sstream>
#include <unordered_map>
#include <iostream>

#include "TRandom3.h"
#include "TH1.h"

template <typename T> class TDistr1D;
using Distr1D    = TDistr1D<double>;
using Distr1Dint = TDistr1D<int>;

class Distr3D;

class RandomManager {
public:
    static RandomManager* Instance(unsigned int seed = 42);
    static void Destroy();
    
    Distr3D* Fact3DJointDistr(const std::string& configStr);
    Distr1D* SetDistr(const std::string& configStr);
    Distr1D* SetDistr(std::stringstream& sStream);
    Distr1Dint* SetDistrInt(const std::string& configStr);

    double Gaus(double mean, double sigma);
    double Uniform(double min, double max);
    double Fixed(double fixedValue);
    int Poisson(double mean);
    
    double EvaluateMath(const std::string& expr);
    TH1* GetStoredHistogram(const std::string& fileName, const std::string& histName);

    static double Module2Pi(double angle);

private:
    RandomManager(unsigned int seed);
    ~RandomManager();

    static RandomManager* fInstance;
    TRandom3* fRandom;

    std::unordered_map<std::string, TH1*> fHistograms; 
};

#endif