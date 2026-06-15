#include <cmath>

#include "Detector.h"
#include "RandomManager.h"
#include "Particle.h"
#include "Event.h"

Detector::Detector() 
    : Cylinder(), fSigmaZ(0), fSigmaRPhi(0), fEfficiency(1.0) {
    fIsActive = true;
}

Detector::Detector(double radius, double length, double thickness, double x0,
                   double sigmaZ, double sigmaRPhi, double efficiency, double noise)
    : Cylinder(radius, length, thickness, x0), fSigmaZ(sigmaZ), fSigmaRPhi(sigmaRPhi), fEfficiency(efficiency) {
    fIsActive = true;
}

void Detector::Load(std::stringstream& ss) {
    Cylinder::Load(ss);
    
    ss >> fSigmaZ >> fSigmaRPhi >> fEfficiency;
    fIsActive = true;
}

bool Detector::Interact(Particle& part, Hit& hitBuffer) {
    Cylinder::Interact(part, hitBuffer);

    double trueZ = part.GetZ();
    double truePhi = part.GetPhi();
    double trueR = fRadius;

    bool isDetected = (fRand->Uniform(0.0, 1.0) <= fEfficiency);

    double measZ = 0.0;
    double measPhi = 0.0;

    if (isDetected) {
        measZ = trueZ + fRand->Gaus(0.0, fSigmaZ);
        
        double dRPhi = fRand->Gaus(0.0, fSigmaRPhi);
        measPhi = truePhi + (dRPhi / trueR);
        measPhi = fRand->Module2Pi(measPhi);
    } else {
        measZ = std::nan("");     
        measPhi = std::nan("");   
    }

    hitBuffer.Set(fLayerID, part.GetTrackID(), trueR, trueZ, truePhi, measZ, measPhi, isDetected);

    return true;
}

REGISTER_GEOMETRY(Detector, "DETECT")