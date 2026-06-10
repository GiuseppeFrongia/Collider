#ifndef PARTICLE_H
#define PARTICLE_H

#include <cmath>
#include <algorithm>
#include <iostream>

class Particle {
public:
    Particle(int trackID, double x, double y, double z, double ux, double uy, double uz, double p)
        : fTrackID(trackID), fX(x), fY(y), fZ(z), fUx(ux), fUy(uy), fUz(uz), fP(p), fIsAlive(true) {
        NormalizeDirection();
    }

    int    GetTrackID() const { return fTrackID; }
    double GetX()       const { return fX; }
    double GetY()       const { return fY; }
    double GetZ()       const { return fZ; }
    double GetUx()      const { return fUx; }
    double GetUy()      const { return fUy; }
    double GetUz()      const { return fUz; }
    double GetP()       const { return fP; }
    bool   IsAlive()    const { return fIsAlive; }

    void Kill() { fIsAlive = false; }
    
    void SetPosition(double x, double y, double z) {
        fX = x; fY = y; fZ = z;
    }

    void SetDirection(double ux, double uy, double uz) {
        fUx = ux; fUy = uy; fUz = uz;
        NormalizeDirection(); // Protezione automatica contro la perdita di unitarietà
    }

    void SetMomentum(double p) { fP = p; }

    double GetR() const { 
        return std::sqrt(fX * fX + fY * fY); 
    }
    
    double GetPhi() const { 
        return std::atan2(fY, fX); 
    }
    
    double GetTheta() const { 
        double cosTheta = std::clamp(fUz, -1.0, 1.0);
        return std::acos(cosTheta); 
    }

    double GetPt() const {
        // Impulso trasverso
        return fP * std::sqrt(fUx * fUx + fUz * fUx);
    }

private:
    int fTrackID;
    
    double fX, fY, fZ;
    
    double fUx, fUy, fUz;
    
    double fP;
    
    bool fIsAlive;

    void NormalizeDirection() {
        double norm2 = fUx * fUx + fUy * fUy + fUz * fUz;
        if (norm2 < 1e-12) {
            fUx = 0.0; fUy = 0.0; fUz = 1.0;
            return;
        }
        double norm = std::sqrt(norm2);
        fUx /= norm;
        fUy /= norm;
        fUz /= norm;
    }
};

#endif // PARTICLE_H