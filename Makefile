CXX = g++
MPICXX = mpic++
HIPCC = hipcc
CXXFLAGS = -O3 -std=c++17
HIP_LIBS ?= -l:libamdhip64.so.7

BIN_DIR = binary
OUT_DIR = output

.PHONY: all all-with-hip serial openmp mpi hip clean

all: serial openmp mpi

all-with-hip: all hip

serial: $(BIN_DIR)/serial
openmp: $(BIN_DIR)/openmp
mpi: $(BIN_DIR)/mpi
hip: $(BIN_DIR)/hip

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/serial: serial.cpp ppm.hpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) serial.cpp -o $@

$(BIN_DIR)/openmp: openmp.cpp ppm.hpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -fopenmp openmp.cpp -o $@

$(BIN_DIR)/mpi: mpi.cpp ppm.hpp | $(BIN_DIR)
	$(MPICXX) $(CXXFLAGS) mpi.cpp -o $@

$(BIN_DIR)/hip: hip.cpp ppm.hpp | $(BIN_DIR)
	$(HIPCC) -O3 -std=c++17 hip.cpp $(HIP_LIBS) -o $@

clean:
	rm -rf $(BIN_DIR) $(OUT_DIR)
