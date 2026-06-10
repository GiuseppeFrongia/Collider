#ifndef DISTRIBUTIONS_H
#define DISTRIBUTIONS_H

#include "RandomManager.h"
#include "TH1.h"

template <typename T>
class TDistr1D {
public:
    virtual ~TDistr1D() = default;
    virtual T Sample() = 0; 
};

using Distr1D    = TDistr1D<double>;
using Distr1Dint = TDistr1D<int>;

class Distr3D {
public:
    virtual ~Distr3D() = default;
    virtual void Sample(double& x, double& y, double& z) = 0;
};

class JointFact3D : public Distr3D {
private:
    Distr1D *fX, *fY, *fZ;
public:
    JointFact3D(Distr1D* x, Distr1D* y, Distr1D* z) : fX(x), fY(y), fZ(z) {}
    ~JointFact3D() override { delete fX; delete fY; delete fZ; }

    void Sample(double& x, double& y, double& z) override {
        x = fX->Sample();
        y = fY->Sample();
        z = fZ->Sample();
    }
};

class GaussianDistr : public Distr1D {
private:
    RandomManager* fRand;
    double fMean, fSigma;
public:
    GaussianDistr(RandomManager* r, double m, double s) : fRand(r), fMean(m), fSigma(s) {}
    double Sample() override { return fRand->Gaus(fMean, fSigma); }
};

class UniformDistr : public Distr1D {
private:
    RandomManager* fRand;
    double fMin, fMax;
public:
    UniformDistr(RandomManager* r, double min, double max) : fRand(r), fMin(min), fMax(max) {}
    double Sample() override { return fRand->Uniform(fMin, fMax); }
};

class FixedDistr : public Distr1D {
private:
    double fValue;
public:
    explicit FixedDistr(double val) : fValue(val) {}
    double Sample() override { return fValue; }
};

class HistDistr : public Distr1D {
private:
    TH1* fHist;
public:
    explicit HistDistr(TH1* hist) : fHist(hist) {}
    
    double Sample() override { 
        if (!fHist) return 0.0;

        return fHist->GetRandom(); 
    }
};


class PoissonDistr : public Distr1Dint {
private:
    RandomManager* fRand;
    double fMean;
public:
    PoissonDistr(RandomManager* r, double m) : fRand(r), fMean(m) {}
    int Sample() override { return fRand->Poisson(fMean); }
};

class UniformDistrInt : public Distr1Dint {
private:
    RandomManager* fRand;
    int fMin, fMax;
public:
    UniformDistrInt(RandomManager* r, int min, int max) : fRand(r), fMin(min), fMax(max) {}
    int Sample() override { return fRand->Uniform(fMin, fMax); }
};

class FixedDistrInt : public Distr1Dint {
private:
    int fValue;
public:
    explicit FixedDistrInt(int val) : fValue(val) {}
    int Sample() override { return fValue; }
};

class HistDistrInt : public Distr1Dint {
private:
    TH1* fHist;
public:
    explicit HistDistrInt(TH1* hist) : fHist(hist) {}
    
    int Sample() override { 
        if (!fHist) return 0;

        return fHist->GetRandom(); 
    }
};

#endif // DISTRIBUTIONS_H