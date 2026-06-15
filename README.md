# Guida alla Compilazione e all'Esecuzione

Questa guida spiega brevemente come compilare ed eseguire il programma utilizzando CMake.

---

## 🛠️ Come Compilare il Programma

Il framework utilizza **CMake** per gestire la compilazione. Seguire questi passaggi da terminale per compilare il codice sorgente:

1. **Crea la cartella di build e ci entra:**
   ```bash
   mkdir -p build && cd build
    ```

2.  **Genera i file di configurazione di Make ed esegue la compilazione:**
    ```bash
    cmake ../source/
    make
    ```

---

## 🚀 Come Eseguire lil Programma

Una volta completata la compilazione con successo, verrà generato l'eseguibile. 

Per avviare l'analisi dati, eseguire il binario:

```bash
./collider -s input/esempio.sim
./collider -r input/esempio.reco
./collider -a input/esempio.ana
```
