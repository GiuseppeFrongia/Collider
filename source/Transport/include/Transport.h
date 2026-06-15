#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <string>
#include <map>
#include <functional>
#include <sstream>

class Particle;
class Cylinder;

class Transport {
public:
    using TransportPlaceholder = std::function<Transport*()>;

    virtual ~Transport() = default;

    virtual void Setup(std::stringstream& ss) {}

    virtual bool Propagate(Particle& part, const Cylinder* targetLayer) = 0;

    static Transport* Create(const std::string& configStr);
    static void RegisterType(const std::string& type, TransportPlaceholder transportClass);

private:
    static std::map<std::string, TransportPlaceholder>& GetRegistry();
};

#define REGISTER_TRANSPORT(ClassName, TypeName) \
    static struct ClassName##Registrar { \
        ClassName##Registrar() { \
            Transport::RegisterType(TypeName, []() { return new ClassName(); }); \
        } \
    } global_##ClassName##_registrar;

#endif // TRANSPORT_H