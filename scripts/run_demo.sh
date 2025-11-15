#!/usr/bin/env bash
set -euo pipefail

NP="${NP:-6}"
MPIRUN_OPTS="${MPIRUN_OPTS:-}"
OUT="${OUT:-out/matriz.csv}"
FILES=(data/*.txt)

# Verifica archivos
if [ ${#FILES[@]} -eq 0 ] || [ ! -e "${FILES[0]}" ]; then
  echo "No hay archivos en data/." >&2
  exit 1
fi

# Compila
make -s serial mpi >/dev/null
mkdir -p "$(dirname "$OUT")"

# Serial: 3 corridas
sum=0.0
for i in 1 2 3; do
  t=$(./bin/bow_serial "${FILES[@]}" --out "$OUT" | awk -F= '/time_sec/{print $2}')
  : "${t:=0}"
  sum=$(awk -v a="$sum" -v b="$t" 'BEGIN{printf "%.10f", a+b}')
done
ts=$(awk -v s="$sum" 'BEGIN{printf "%.6f", s/3.0}')

# MPI: 3 corridas
sum=0.0
tmpfile=$(mktemp)
trap 'rm -f "$tmpfile"' EXIT
for i in 1 2 3; do
  mpirun $MPIRUN_OPTS -np "$NP" ./bin/bow_mpi "${FILES[@]}" --out "$OUT" >"$tmpfile"
  t=$(awk -F= '/time_sec/{print $2}' "$tmpfile")
  : "${t:=0}"
  sum=$(awk -v a="$sum" -v b="$t" 'BEGIN{printf "%.10f", a+b}')
done
tp=$(awk -v s="$sum" 'BEGIN{printf "%.6f", s/3.0}')

# Speed-up
speed=$(awk -v a="$ts" -v b="$tp" 'BEGIN{if (b>0) printf "%.6f", a/b; else print 0}')

echo "serial_avg_sec=$ts"
echo "mpi_avg_sec=$tp"
echo "speedup=$speed"
