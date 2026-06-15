#include "InputManager.h"
#include <fstream>
#include <algorithm>

InputManager::InputManager(const std::string& filename) : fLoaded(false) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open input file: " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
            line = Trim(line);
        }

        if (line.empty()) continue;

        // cerca la posizione del separatore '='
        size_t delimPos = line.find('=');
        if (delimPos == std::string::npos) continue; // salta righe senza '='

        // estrae la chiave (sinistra) e il valore (destra) isolando l'uguale
        std::string key = Trim(line.substr(0, delimPos));
        std::string value = Trim(line.substr(delimPos + 1));

        // inserisce nella mappa. Se la chiave è duplicata, sovrascrive
        if (!key.empty()) {
            fParameters[key] = value;
        }
    }

    fLoaded = true;
}

std::string InputManager::Trim(const std::string& str) const {
    // gestisce spazi, tabulazioni (\t) e i ritorni a capo di Windows (\r, \n)
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string InputManager::GetRawString(const std::string& key, const std::string& defaultVal) const {
    auto it = fParameters.find(key);
    
    if (it != fParameters.end()) {
        return it->second;
    }
    return defaultVal;
}