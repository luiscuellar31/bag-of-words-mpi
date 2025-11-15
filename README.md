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
- Ejecuta el script de demo (recomendado): `NP=<#procesos> scripts/run_demo.sh`.
- El script compila, corre 3× serial y 3× MPI, genera `out/matriz.csv` y muestra `speedup=...`.
- Usa `NP` igual al número de archivos (ejemplo: 6 libros → `NP=6`).
- Ejecución manual:
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
  - Speed-up = T_serial / T_paralelo, medido y reportado por `scripts/run_demo.sh` (promedios de 3 corridas).

## Compilación
- `make serial` genera `bin/bow_serial`
- `make mpi` genera `bin/bow_mpi`
- `make clean` limpia artefactos

 
## Formato CSV
- Cabecera: primera columna `doc`, luego términos del vocabulario global ordenado.
- Filas: una por documento, con frecuencias por término.

## Medición de tiempo
- Cada binario imprime el tiempo total en segundos como `time_sec=...`.
- Script de demo: `scripts/run_demo.sh` compila, ejecuta 3 corridas serial y 3 corridas MPI, promedia tiempos y muestra el speed-up.
- Uso: `NP=6 scripts/run_demo.sh` (ajusta `NP` al número de procesos deseado).
- Salida del script: `serial_avg_sec=...`, `mpi_avg_sec=...`, `speedup=...`.
- Nota: el CSV `out/matriz.csv` se reescribe en cada corrida del demo.

## Datos de prueba
Se incluyen `data/doc1.txt` y `data/doc2.txt` y un `data/README.md`.

## Validación esperada
- Ejecutar el mismo conjunto de libros en serial y en MPI debe producir el mismo CSV (mismo vocabulario y orden de columnas). El vocabulario global se ordena para garantizar comparabilidad.
- Para el caso canónico del curso (N libros / N procesos), usa `mpirun -np N` con N igual al número de archivos.

## Recomendaciones
- Coloca textos de prueba más grandes en `data/` para medir speed-up de forma representativa. El objetivo sugerido por el curso es superar 1.2×.
- Si tu entorno MPI requiere configuración de red, asegúrate de que `mpirun` funcione correctamente antes de medir.
