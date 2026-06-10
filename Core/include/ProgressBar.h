#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include <algorithm>

// Librerie POSIX per leggere le dimensioni del terminale su Linux/Mac
#include <sys/ioctl.h>
#include <unistd.h>

class ProgressBar {
public:
    ProgressBar(int totalSteps, const std::string& taskName = "Process")
        : fTotal(totalSteps), fTaskName(taskName), fLastPercent(-1) {}

    void Start() {
        fStartTime = std::chrono::steady_clock::now();
        std::cout << "[INFO] Starting " << fTaskName << std::endl;
    }

    void Update(int currentStep) {
        int percent = static_cast<int>((currentStep * 100.0) / fTotal);

        if (percent > fLastPercent || currentStep == fTotal) {
            fLastPercent = percent;
            Print(currentStep, percent);
        }
    }

private:
    int fTotal;
    int fLastPercent;
    std::string fTaskName;
    std::chrono::time_point<std::chrono::steady_clock> fStartTime;

    int GetTerminalWidth() const {
        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
            return w.ws_col;
        }
        return 80;
    }

    void Print(int current, int percent) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - fStartTime;

        double eta = 0.0;
        if (current > 0) {
            eta = (elapsed.count() / current) * (fTotal - current);
        }

        char diagBuffer[100];
        snprintf(diagBuffer, sizeof(diagBuffer), "] %3d%% | Elapsed: %.1fs | ETA: %.1fs",
                 percent, elapsed.count(), std::max(0.0, eta));
        std::string diagText = diagBuffer;

        int termWidth = GetTerminalWidth();
        
        int barWidth = termWidth - fTaskName.length() - diagText.length() - 4;
        
        if (barWidth < 10) barWidth = 10; 

        int pos = static_cast<int>(barWidth * (current / static_cast<double>(fTotal)));

        std::cout << "\r" << fTaskName << " [";
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        
        std::cout << diagText << std::flush;

        if (current == fTotal) {
            std::cout << "\n[INFO] " << fTaskName << " completed in " 
                      << std::fixed << std::setprecision(1) << elapsed.count() << " s." << std::endl;
        }
    }
};

#endif // PROGRESSBAR_H