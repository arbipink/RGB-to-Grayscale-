# Konversi RGB ke Grayscale (Serial, OpenMP, MPI, dan ROCm/HIP)

Proyek ini mengimplementasikan konversi gambar RGB berformat PPM (`P6`) menjadi gambar grayscale berformat PGM (`P5`) menggunakan empat pendekatan komputasi: **Serial**, **OpenMP**, **MPI**, dan **ROCm/HIP**.

Formula konversi yang digunakan adalah standar luminansi ITU-R BT.601:

```
Gray = 0.299 * R + 0.587 * G + 0.114 * B
```

## Struktur Proyek

```
.
├── ppm.hpp          # Helper I/O PPM/PGM, konversi piksel, dan konstanta bersama
├── serial.cpp       # Implementasi Serial (single-thread CPU)
├── openmp.cpp       # Implementasi OpenMP (multi-thread CPU)
├── mpi.cpp          # Implementasi MPI (distributed-memory CPU)
├── hip.cpp          # Implementasi ROCm/HIP (GPU)
├── Makefile         # Build system (selalu mengarahkan binary ke folder binary/)
├── run_all.sh       # Script kompilasi & pengujian seluruh varian
├── rgbImage.ppm     # Gambar input uji (1920 x 1280 piksel, 7.372.817 byte)
├── binary/          # Folder output seluruh file binary hasil kompilasi
│   ├── serial
│   ├── openmp
│   ├── mpi
│   └── hip
└── output/          # Folder output gambar hasil konversi (.pgm)
    ├── gray_serial.pgm
    ├── gray_openmp.pgm
    ├── gray_mpi.pgm
    └── gray_hip.pgm
```

## Cara Build & Menjalankan

### Menggunakan Script Otomatis

```bash
./run_all.sh
```

Atau dengan file input dan konfigurasi thread/proses khusus:

```bash
OMP_NUM_THREADS=4 MPI_PROCESSES=4 ./run_all.sh rgbImage.ppm
```

### Build Manual dengan Makefile

```bash
# Build Serial, OpenMP, dan MPI ke folder binary/
make

# Build seluruhnya termasuk ROCm/HIP ke folder binary/
make all-with-hip

# Recompile paksa seluruh target ke folder binary/
make -B all-with-hip

# Hapus folder binary/ dan output/
make clean
```

### Menjalankan Binary Secara Terpisah

```bash
./binary/serial rgbImage.ppm output/gray_serial.pgm
OMP_NUM_THREADS=4 ./binary/openmp rgbImage.ppm output/gray_openmp.pgm
mpirun -np 4 ./binary/mpi rgbImage.ppm output/gray_mpi.pgm
./binary/hip rgbImage.ppm output/gray_hip.pgm
```

## Hasil Pengujian

### Lingkungan Uji
- **CPU**: AMD Ryzen 5 6600H with Radeon Graphics (12 thread/CPU logis)
- **GPU**: AMD Radeon Graphics (`gfx1035`)
- **Gambar Input**: `rgbImage.ppm` — resolusi `1920 x 1280` (`2.457.600` piksel)
- **Konfigurasi**: `OMP_NUM_THREADS=4`, `MPI_PROCESSES=4`

### Waktu Eksekusi

| Varian | Konfigurasi | Waktu Eksekusi | Cakupan Pengukuran |
| :--- | :--- | :---: | :--- |
| **Serial** | 1 thread | `0.00333038 s` | Loop konversi RGB ke Grayscale di CPU |
| **OpenMP** | 4 thread | `0.00138036 s` | Loop paralel `#pragma omp parallel for` di CPU |
| **MPI** | 4 proses | `0.00271719 s` | `MPI_Scatterv` + konversi lokal + `MPI_Gatherv` |
| **ROCm/HIP** | GPU (`gfx1035`) | `0.297725 s` | `hipMemcpy` (H2D) + kernel + sinkronisasi + `hipMemcpy` (D2H) |

### Analisis Singkat
- **OpenMP** mencatat waktu tercepat (`0.00138036 s`, ~2,41x lebih cepat dari Serial) karena seluruh piksel sudah berada di memori bersama (*shared memory*) tanpa overhead komunikasi antar-proses maupun transfer host-device.
- **MPI** (`0.00271719 s`) lebih cepat daripada Serial, namun lebih lambat dari OpenMP karena waktu yang diukur mencakup distribusi dan pengumpulan data (`MPI_Scatterv` dan `MPI_Gatherv`) antar 4 proses.
- **ROCm/HIP** (`0.297725 s`) terlihat lebih lama pada pengukuran end-to-end untuk ukuran gambar 1920×1280 karena waktu yang dicatat mencakup inisialisasi runtime/code object pertama kali pada peluncuran kernel serta transfer memori host-device (`hipMemcpy` H2D dan D2H), sementara komputasi per piksel itu sendiri sangat ringan.
