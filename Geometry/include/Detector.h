#ifndef DETECTOR_H
#define DETECTOR_H

#include "Cylinder.h"
#include <sstream>

class Particle;
class Hit;

class Detector : public Cylinder {
public:
    Detector();
    Detector(double radius, double length, double thickness, double x0, 
             double sigmaZ, double sigmaRPhi, double efficiency, double noise);
    virtual ~Detector() = default;

    void Load(std::stringstream& ss) override;
    
    bool Interact(Particle& part, Hit& hitBuffer) override;

    double GetNoise() const { return fNoise; }
    double GetEfficiency() const { return fEfficiency; }

private:
    double fSigmaZ;
    double fSigmaRPhi;
    double fEfficiency;
    double fNoise;
};

#endif // DETECTOR_H