# Bolsa de Palabras (C++20, Serial y MPI)

Proyecto base para construir una matriz documento–término desde archivos de texto y exportarla a CSV. Incluye versiones serial y MPI con el mismo orden de columnas (vocabulario global ordenado) para comparar salidas y tiempos.

## Requisitos
- C++20 (`g++`)
- MPI (`mpic++`, `mpirun`)

## Estructura
- `include/`: cabeceras (`tokenize.hpp`, `csv.hpp`, `timer.hpp`)
- `src/`: implementaciones compartidas (`tokenize.cpp`, `csv.cpp`)
- `serial/`: `main_serial.cpp`
- `mpi/`: `main_mpi.cpp`
- `data/`: coloca aquí los `.txt` de prueba (se incluyen 2 pequeños)
- `out/`: CSV de salida
- `scripts/`: utilidades (ejecución y medición)

## Compilación
- `make serial` genera `bin/bow_serial`
- `make mpi` genera `bin/bow_mpi`
- `make clean` limpia artefactos

## Uso
Ejemplos con los archivos de `data/`:
- `./bin/bow_serial data/*.txt --out out/matriz.csv`
- `mpirun -np 6 ./bin/bow_mpi data/*.txt --out out/matriz.csv`

Ambos aceptan `--out RUTA.csv` y el resto son rutas a `.txt`.

## Formato CSV
- Cabecera: primera columna `doc`, luego términos del vocabulario global ordenado.
- Filas: una por documento, con frecuencias por término.

## Medición de tiempo
- Cada binario imprime el tiempo total en segundos como `time_sec=...`.
- `scripts/run_demo.sh` ejecuta 3 corridas serial y MPI, promedia y muestra speed-up.

## Datos de prueba
Se incluyen `data/doc1.txt` y `data/doc2.txt` y un `data/README.md`.
