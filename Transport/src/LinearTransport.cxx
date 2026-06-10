#include <cmath>
#include <iostream>

#include "LinearTransport.h"
#include "Particle.h"
#include "Cylinder.h"

bool LinearTransport::Propagate(Particle& part, const Cylinder* targetLayer) {
    if (!targetLayer) return false;

    double R = targetLayer->GetRadius();
    double halfLength = targetLayer->GetLength() / 2.0;

    double x0 = part.GetX();
    double y0 = part.GetY();
    double z0 = part.GetZ();

    double ux = part.GetUx();
    double uy = part.GetUy();
    double uz = part.GetUz();

    double a = ux * ux + uy * uy;
    
    if (a < 1e-12) return false;

    double b = (x0 * ux + y0 * uy);
    double c = x0 * x0 + y0 * y0 - R * R;

    double delta = b * b - a * c;
    
    if (delta < 0.0) return false;

    double sqrtDelta = std::sqrt(delta);
    double t1 = (-b - sqrtDelta) / a;
    double t2 = (-b + sqrtDelta) / a;

    double t = -1.0;
    if (t1 > 1e-6) t = t1;
    else if (t2 > 1e-6) t = t2;

    if (t < 0.0) return false;

    double x_hit = x0 + t * ux;
    double y_hit = y0 + t * uy;
    double z_hit = z0 + t * uz;

    if (std::abs(z_hit) > halfLength) return false;

    part.SetPosition(x_hit, y_hit, z_hit);
    return true;
}

REGISTER_TRANSPORT(LinearTransport, "LinearTransport")