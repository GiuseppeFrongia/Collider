#include <iostream>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>

#include "SimulationManager.h"
#include "ReconstructionManager.h"
#include "AnalysisManager.h"
// #include "RunRegistry.h"

int main(int argc, char* argv[]) {                      //parametri CLI per automazione
    if (argc > 1) {                                     //lancio di singola operazione
        std::string flag = argv[1];
        if (flag == "-s" && argc >= 3) {
            SimulationManager sim; if (sim.Init(argv[2])) { sim.Run(); return 0; } return 1;
        }
        if (flag == "-r" && argc >= 3) {
            ReconstructionManager reco; if (reco.Init(argv[2])) { reco.Run(); return 0; } return 1;
        }
        if (flag == "-a" && argc >= 3) {
            AnalysisManager ana; if (ana.Init(argv[2])) { ana.Run(); return 0; } return 1;
        }
        std::cerr << "Flag unknown. Use -s (simulation), -r (reconstruction) or -a (analysis)." << std::endl; return 1;
    }

    if (!isatty(STDIN_FILENO)) {                        //controlla che il programma sia chiamato in una pipeline 
        int choice;
        std::string customPath;
        
        while (std::cin >> choice) {                    //guarda la lista delle operazioni da eseguire
            if (choice == 1) {
                std::cin >> customPath;                 //salva l'indirezzo del file di input per una determinata operazione
                SimulationManager sim;
                if (sim.Init(customPath)) sim.Run();    //inizializza e esegue (guardare codice per ogni operazione)
                
            } else if (choice == 2) {
                std::cin >> customPath;
                ReconstructionManager reco; 
                if (reco.Init(customPath)) reco.Run();
                
            } else if (choice == 3) {
                std::cin >> customPath;
                AnalysisManager ana; 
                if (ana.Init(customPath)) ana.Run();
                
            } else if (choice == 0) {                   //alla fine del file di input concatenato è bene mettere "la chiusura"
                break;
            }
        }
        return 0;
    }

                                                        //stringa finale al posto del menu interattivo.
    std::cout << "Please, launch the program with an input file." << std::endl;
    return 0;
}
