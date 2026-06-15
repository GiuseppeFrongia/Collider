#include "Transport.h"

std::map<std::string, Transport::TransportPlaceholder>& Transport::GetRegistry() {
    static std::map<std::string, TransportPlaceholder> registry;
    return registry;
}

void Transport::RegisterType(const std::string& type, TransportPlaceholder transportClass) {
    GetRegistry()[type] = transportClass;
}

Transport* Transport::Create(const std::string& configStr) {
    std::stringstream ss(configStr);
    std::string type;
    
    ss >> type; 

    auto& reg = GetRegistry();
    auto it = reg.find(type);
    
    if (it != reg.end()) {
        Transport* transportClass = it->second();
        
        transportClass->Setup(ss); 
        
        return transportClass;
    }
    
    return nullptr;
}