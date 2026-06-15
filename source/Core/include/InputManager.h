#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <string>
#include <map>
#include <sstream>
#include <iostream>

class InputManager {
public:
    InputManager(const std::string& filename);
    ~InputManager() = default;

    bool IsLoaded() const { return fLoaded; }

    std::string GetRawString(const std::string& key, const std::string& defaultVal) const;

    template <typename T>
    T GetParameter(const std::string& key, T defVal) const {
        auto it = fParameters.find(key);
        
        if (it == fParameters.end()) {
            std::cerr << "[WARNING] Key '" << key 
                      << "' not found in input file. Using default: " 
                      << defVal << std::endl;
            return defVal;
        }

        std::stringstream ss(it->second);
        T result;
        if (ss >> result) {
            return result;
        }
        
        std::cerr << "[WARNING] Conversion failed for key '" << key 
                  << "' (provided value: '" << it->second 
                  << "'). Falling back to default: " << defVal << std::endl;
                  
        return defVal;
    }

private:
    bool fLoaded;
    std::map<std::string, std::string> fParameters;
    std::string Trim(const std::string& str) const;
};

template <>
inline bool InputManager::GetParameter<bool>(const std::string& key, bool defVal) const {
    auto it = fParameters.find(key);
    
    if (it == fParameters.end()) {
        std::cerr << "[WARNING] Key '" << key 
                  << "' not found in input file. Using default: " 
                  << std::boolalpha << defVal << std::endl;
        return defVal;
    }

    std::string val = it->second;
    
    if (val == "true" || val == "1" || val == "yes" || val == "TRUE" || val == "ON") {
        return true;
    }
    if (val == "false" || val == "0" || val == "no" || val == "FALSE" || val == "OFF") {
        return false;
    }

    std::cerr << "[WARNING] Invalid boolean value '" << val 
              << "' for key '" << key 
              << "'. Falling back to default: " 
              << std::boolalpha << defVal << std::endl;
              
    return defVal;
}

#endif
