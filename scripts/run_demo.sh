#!/usr/bin/env bash
set -euo pipefail

NP="${NP:-3}"
MPIRUN_OPTS="${MPIRUN_OPTS:-}"
OUT="${OUT:-out/matriz.csv}"
RUNS="${RUNS:-10}"
FILES=(data/*.txt)

if [ "$RUNS" -lt 1 ]; then RUNS=1; fi

# Verifica archivos
if [ ${#FILES[@]} -eq 0 ] || [ ! -e "${FILES[0]}" ]; then
  echo "No hay archivos en data/." >&2
  exit 1
fi

# Compila
make -s serial mpi >/dev/null
mkdir -p "$(dirname "$OUT")"

# Serial: promedio RUNS corridas
sum=0.0
for ((i=1; i<=RUNS; ++i)); do
  t=$(./bin/bow_serial "${FILES[@]}" --out "$OUT" | awk -F= '/time_sec/{print $2}')
  : "${t:=0}"
  sum=$(awk -v a="$sum" -v b="$t" 'BEGIN{printf "%.10f", a+b}')
done
ts=$(awk -v s="$sum" -v n="$RUNS" 'BEGIN{printf "%.6f", s/n}')

# MPI: promedio RUNS corridas
sum=0.0
tmpfile=$(mktemp)
trap 'rm -f "$tmpfile"' EXIT
for ((i=1; i<=RUNS; ++i)); do
  mpirun $MPIRUN_OPTS -np "$NP" ./bin/bow_mpi "${FILES[@]}" --out "$OUT" >"$tmpfile"
  t=$(awk -F= '/time_sec/{print $2}' "$tmpfile")
  : "${t:=0}"
  sum=$(awk -v a="$sum" -v b="$t" 'BEGIN{printf "%.10f", a+b}')
done
tp=$(awk -v s="$sum" -v n="$RUNS" 'BEGIN{printf "%.6f", s/n}')

# Speed-up
speed=$(awk -v a="$ts" -v b="$tp" 'BEGIN{if (b>0) printf "%.6f", a/b; else print 0}')

echo "runs=$RUNS"
echo "P=$NP"
echo "T_serial_avg_sec=$ts"
echo "T_paralelo_avg_sec(P=$NP)=$tp"
echo "speedup=$speed"
