#include "Detector.h"
#include "RandomManager.h"
#include "Particle.h"
#include "Event.h"
#include <cmath>

Detector::Detector() 
    : Cylinder(), fSigmaZ(0), fSigmaRPhi(0), fEfficiency(1.0), fNoise(0) {
    fIsActive = true;
}

Detector::Detector(double radius, double length, double thickness, double x0,
                   double sigmaZ, double sigmaRPhi, double efficiency, double noise)
    : Cylinder(radius, length, thickness, x0), fSigmaZ(sigmaZ), fSigmaRPhi(sigmaRPhi),
      fEfficiency(efficiency), fNoise(noise) {
    fIsActive = true;
}

void Detector::Load(std::stringstream& ss) {
    Cylinder::Load(ss);
    
    ss >> fSigmaZ >> fSigmaRPhi >> fEfficiency >> fNoise;
    fIsActive = true;
}

bool Detector::Interact(Particle& part, Hit& hitBuffer) {
    Cylinder::Interact(part, hitBuffer);

    double true_z = part.GetZ();
    double true_phi = part.GetPhi();
    double true_r = fRadius;

    bool isDetected = (fRand->Uniform(0.0, 1.0) <= fEfficiency);

    double meas_z = 0.0;
    double meas_phi = 0.0;

    if (isDetected) {
        meas_z = true_z + fRand->Gaus(0.0, fSigmaZ);
        
        double dRPhi = fRand->Gaus(0.0, fSigmaRPhi);
        meas_phi = true_phi + (dRPhi / true_r);
        meas_phi = fRand->Module2Pi(meas_phi);
    } else {
        meas_z = std::nan("");     
        meas_phi = std::nan("");   
    }

    hitBuffer.Set(fLayerID, part.GetTrackID(), true_r, true_z, true_phi, meas_z, meas_phi, isDetected);

    return true;
}

REGISTER_GEOMETRY(Detector, "DETECT")