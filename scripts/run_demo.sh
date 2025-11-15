#!/usr/bin/env bash
set -euo pipefail
NP="${NP:-6}"
OUT="out/matriz.csv"
FILES=(data/*.txt)
if [ ${#FILES[@]} -eq 0 ]; then
  echo "No hay archivos en data/." >&2
  exit 1
fi
make -s serial mpi >/dev/null
mkdir -p out
sum=0
for i in 1 2 3; do
  t=$(./bin/bow_serial "${FILES[@]}" --out "$OUT" | awk -F= '/time_sec/{print $2}')
  sum=$(python3 - <<PY
import sys
print({} + float(sys.argv[1]))
PY
$sum $t)
done
ts=$(python3 - <<PY
print(round(({})/3.0,6))
PY
$sum)
sum=0
for i in 1 2 3; do
  TSERIAL_SEC=$ts mpirun -np "$NP" ./bin/bow_mpi "${FILES[@]}" --out "$OUT" >/tmp/mpi_out.$$ 
  t=$(awk -F= '/time_sec/{print $2}' /tmp/mpi_out.$$)
  sum=$(python3 - <<PY
import sys
print({} + float(sys.argv[1]))
PY
$sum $t)
done
tp=$(python3 - <<PY
print(round(({})/3.0,6))
PY
$sum)
speed=$(python3 - <<PY
import sys
ts, tp = map(float, sys.argv[1:3])
print(round(ts/tp, 6) if tp>0 else 0)
PY
$ts $tp)
echo "serial_avg_sec=$ts"
echo "mpi_avg_sec=$tp"
echo "speedup=$speed"

