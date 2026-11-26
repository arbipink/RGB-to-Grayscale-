#!/usr/bin/env bash
set -e

INPUT="${1:-rgbImage.ppm}"
BIN_DIR="binary"
OUT_DIR="output"

if [ ! -f "$INPUT" ]; then
    echo "File input tidak ditemukan: $INPUT"
    exit 1
fi

mkdir -p "$BIN_DIR" "$OUT_DIR"

make

if command -v hipcc >/dev/null 2>&1; then
    make hip
else
    echo "hipcc tidak ditemukan; pengujian HIP dilewati."
fi

echo "== Serial =="
"./$BIN_DIR/serial" "$INPUT" "$OUT_DIR/gray_serial.pgm"
echo

echo "== OpenMP =="
OMP_NUM_THREADS="${OMP_NUM_THREADS:-4}" "./$BIN_DIR/openmp" "$INPUT" "$OUT_DIR/gray_openmp.pgm"
echo

echo "== MPI =="
MPI_PROCESSES="${MPI_PROCESSES:-4}"
mpirun -np "$MPI_PROCESSES" "./$BIN_DIR/mpi" "$INPUT" "$OUT_DIR/gray_mpi.pgm"
echo

if [ -x "./$BIN_DIR/hip" ]; then
    echo "== ROCm/HIP =="
    "./$BIN_DIR/hip" "$INPUT" "$OUT_DIR/gray_hip.pgm"
    echo
else
    echo "== ROCm/HIP dilewati =="
    echo
fi

echo "Pengujian selesai. Output: $OUT_DIR/gray_*.pgm"
