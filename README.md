# Bolsa de Palabras (C++20, Serial y MPI)

Proyecto base para construir una matriz documento–término desde archivos de texto y exportarla a CSV. Incluye versiones serial y MPI con el mismo orden de columnas (vocabulario global ordenado) para comparar salidas y tiempos.

## Requisitos
- C++20
- MPI

## Estructura
- `include/`: cabeceras (`tokenize.hpp`, `csv.hpp`, `timer.hpp`)
- `src/`: implementaciones compartidas (`tokenize.cpp`, `csv.cpp`)
- `serial/`: `main_serial.cpp`
- `mpi/`: `main_mpi.cpp`
- `data/`: coloca aquí los `.txt` de prueba (se incluyen 2 pequeños)
- `out/`: CSV de salida
- `scripts/`: utilidades (ejecución y medición)

## Cómo Usarlo
- Coloca tus archivos `.txt` en `data/`.
- Ejecuta el script de demostración (recomendado):
  ```bash
  scripts/run_demo.sh              # usa NP=3 por defecto
  NP=3 scripts/run_demo.sh         # especifica número de procesos
  ```
- El script compila, corre RUNS× serial y RUNS× MPI (por defecto RUNS=100, configurable), genera `out/matriz.csv` y reporta `T_serial_avg_sec`, `T_paralelo_avg_sec(P=...)` y `speedup`.
- Usa `NP` igual al número de archivos (ejemplo: 6 libros → `NP=6`).
- Variables opcionales:
  - `OUT=out/otra_matriz.csv` para cambiar la ruta del CSV.
  - `MPIRUN_OPTS="..."` para pasar flags a `mpirun` si tu entorno lo requiere.
  - `RUNS=3` (o cualquier entero ≥1) para ajustar el número de corridas promedio.
- Ejecución manual (opcional):
  ```bash
  ./bin/bow_serial data/*.txt --out out/matriz.csv
  mpirun -np 6 ./bin/bow_mpi data/*.txt --out out/matriz.csv
  ```
## Qué entrega el proyecto
- Dos programas:
  - Versión serial (una sola CPU): `bin/bow_serial`.
  - Versión paralela con MPI (múltiples procesos): `bin/bow_mpi`.
- Entrada:
  - Lista de archivos `.txt` a procesar (libros/documentos).
  - Número de procesos (para MPI) vía `mpirun -np P`.
- Salida:
  - Archivo CSV con la matriz documento–término (frecuencias enteras) en `out/matriz.csv` (o la ruta indicada con `--out`).
- Métrica de desempeño:
- Speed-up = T_serial / T_paralelo, medido y reportado por `scripts/run_demo.sh` (promedios de RUNS corridas, por defecto 100).

## Compilación
- `make serial` genera `bin/bow_serial`
- `make mpi` genera `bin/bow_mpi`
- `make clean` limpia artefactos

 
## Formato CSV
- Cabecera: primera columna `doc`, luego términos del vocabulario global ordenado.
- Filas: una por documento, con frecuencias por término.

## Medición de tiempo
- Cada binario imprime el tiempo total en segundos como `time_sec=...`.
- Script de demo: `scripts/run_demo.sh` compila, ejecuta RUNS corridas serial y RUNS corridas MPI (por defecto 100) y promedia los tiempos para reportar `speedup`.
- Uso: `NP=<P> scripts/run_demo.sh` (ajusta `NP` al número de procesos deseado).
- Salida del script: `runs=...`, `P=...`, `T_serial_avg_sec=...`, `T_paralelo_avg_sec(P=...)`, `speedup=...`.
- Nota: el CSV `out/matriz.csv` se reescribe en cada corrida del demo.

## Datos de prueba
Se incluyen `data/doc1.txt` y `data/doc2.txt` y un `data/README.md`.

## Validación esperada
- Ejecutar el mismo conjunto de libros en serial y en MPI debe producir el mismo CSV (mismo vocabulario y orden de columnas). El vocabulario global se ordena para garantizar comparabilidad.
- Para el caso canónico del curso (N libros / N procesos), usa `mpirun -np N` con N igual al número de archivos.

## Recomendaciones
- Coloca textos de prueba más grandes en `data/` para medir speed-up de forma representativa. El objetivo sugerido por el curso es superar 1.2×.
- Si tu entorno MPI requiere configuración de red, asegúrate de que `mpirun` funcione correctamente antes de medir.
