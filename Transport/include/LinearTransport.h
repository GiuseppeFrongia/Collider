#ifndef LINEARTRANSPORT_H
#define LINEARTRANSPORT_H

#include "Transport.h"

class LinearTransport : public Transport {
public:
    LinearTransport() = default;
    ~LinearTransport() override = default;

    bool Propagate(Particle& part, const Cylinder* targetLayer) override;
};

#endif // LINEARTRANSPORT_H