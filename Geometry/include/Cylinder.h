#ifndef CYLINDER_H
#define CYLINDER_H

#include <string>
#include <map>
#include <functional>
#include <sstream>

class RandomManager;
class Particle;
class Hit;

class Cylinder {
public:
    using CylinderPlaceholder = std::function<Cylinder*()>;

    Cylinder(double radius=0, double length=0, double thickness=0, double x0=0);
    
    virtual ~Cylinder() = default;
    
    virtual void Load(std::stringstream& ss);

    static Cylinder* Create(const std::string& type);
    static void RegisterType(const std::string& type, CylinderPlaceholder cylinderClass);

    virtual bool Interact(Particle& part, Hit& hitBuffer);

    int GetLayerID() const { return fLayerID; }
    double GetRadius() const { return fRadius; }
    double GetLength() const { return fLength; }
    bool IsActive() const { return fIsActive; }

    void SetLayerID(int layerID) { fLayerID = layerID; }
    void SetMSswitch(bool MSswitch) { fMSswitch = MSswitch; }

protected:
    int fLayerID;
    double fRadius;
    double fLength;
    double fThickness;
    double fX0;
    bool fIsActive;
    bool fMSswitch;

    RandomManager* fRand;

    void ApplyMultipleScattering(Particle& part);

private:
    static std::map<std::string, CylinderPlaceholder>& GetRegistry();
};

#define REGISTER_GEOMETRY(ClassName, TypeName) \
    static struct ClassName##Registrar { \
        ClassName##Registrar() { \
            Cylinder::RegisterType(TypeName, []() { return new ClassName(); }); \
        } \
    } global_##ClassName##_registrar;

#endif // CYLINDER_H