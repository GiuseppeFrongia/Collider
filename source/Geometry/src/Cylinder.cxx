#include <cmath>
#include <iostream>

#include "TMath.h"

#include "RandomManager.h"
#include "Cylinder.h"
#include "Particle.h"
#include "Event.h"

std::map<std::string, Cylinder::CylinderPlaceholder>& Cylinder::GetRegistry() {
    static std::map<std::string, CylinderPlaceholder> registry;
    return registry;
}

void Cylinder::RegisterType(const std::string& type, CylinderPlaceholder cylinderClass) {
    GetRegistry()[type] = cylinderClass;
}

Cylinder* Cylinder::Create(const std::string& type) {
    auto& reg = GetRegistry();
    auto it = reg.find(type);
    if (it != reg.end()) return it->second();
    return nullptr;
}

Cylinder::Cylinder(double radius, double length, double thickness, double x0)
    : fRadius(radius), fLength(length), fThickness(thickness), fX0(x0), fIsActive(false) {
        fRand = RandomManager::Instance();
    }

void Cylinder::Load(std::stringstream& ss) {
    ss >> fRadius >> fLength >> fThickness >> fX0;
}

bool Cylinder::Interact(Particle& part, Hit& hitBuffer) {
    if(fMSswitch) ApplyMultipleScattering(part);
    
    return false;
}

void Cylinder::ApplyMultipleScattering(Particle& part) {
    if (fThickness <= 0.0 || fX0 <= 0.0) return;

    double p = part.GetP();
    if (p < 1e-6) { part.Kill(); return; }

    double x = part.GetX();
    double y = part.GetY();
    double R = fRadius;

    double xnorm = x / R;
    double ynorm = y / R;

    double ux = part.GetUx();
    double uy = part.GetUy();
    double uz = part.GetUz();

    double dotProduct = std::abs(xnorm * ux + ynorm * uy);
    if (dotProduct < 1e-6) dotProduct = 1e-6; 

    double thicknessOverX0 = fThickness / (fX0 * dotProduct);

    // formula di Highland per MS
    double theta0 = (0.0136 / p) * std::sqrt(thicknessOverX0) * (1.0 + 0.038 * std::log(thicknessOverX0));

    // distribuzione di Rayleigh = "exp" * uniforme 
    double dTheta = std::sqrt(fRand->Exp(2 * theta0 * theta0));
    double dPhi = fRand->Uniform(0, TMath::TwoPi());

    double thetaGlob = part.GetTheta();
    double phiGlob = part.GetPhi();

    double ms[3][3];
    ms[0][0] = -std::sin(phiGlob);
    ms[0][1] = -std::cos(thetaGlob) * std::cos(phiGlob);
    ms[0][2] =  std::sin(thetaGlob) * std::cos(phiGlob);
    
    ms[1][0] =  std::cos(phiGlob);
    ms[1][1] = -std::cos(thetaGlob) * std::sin(phiGlob);
    ms[1][2] =  std::sin(thetaGlob) * std::sin(phiGlob);
    
    ms[2][0] =  0.0;
    ms[2][1] =  std::sin(thetaGlob);
    ms[2][2] =  std::cos(thetaGlob);

    double dC[3] = {
        std::sin(dTheta) * std::cos(dPhi), 
        std::sin(dTheta) * std::sin(dPhi), 
        std::cos(dTheta)
    };

    double newUx = 0.0;
    double newUy = 0.0;
    double newUz = 0.0;

    for (int i = 0; i < 3; i++) {
        newUx += ms[0][i] * dC[i];
        newUy += ms[1][i] * dC[i];
        newUz += ms[2][i] * dC[i];
    }

    part.SetDirection(newUx, newUy, newUz);
}

REGISTER_GEOMETRY(Cylinder, "PASSIVE")