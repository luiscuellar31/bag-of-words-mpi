CXX=g++
MPICXX=mpic++
CXXFLAGS=-O2 -std=gnu++20 -Wall -Wextra
INCLUDES=-Iinclude

BIN_DIR=bin
OUT_DIR=out
OBJ_DIR=build/obj

# Fuentes comunes bajo src/
COMMON_SRC=src/tokenize.cpp src/csv.cpp
COMMON_OBJ=$(patsubst %.cpp,$(OBJ_DIR)/%.o,$(COMMON_SRC))

# Objetos de cada binario
SERIAL_OBJ=$(OBJ_DIR)/serial/main_serial.o
MPI_OBJ=$(OBJ_DIR)/mpi/main_mpi.o

.PHONY: all serial mpi clean

all: serial mpi

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

serial: $(BIN_DIR) $(OUT_DIR) $(COMMON_OBJ) $(SERIAL_OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(BIN_DIR)/bow_serial $(COMMON_OBJ) $(SERIAL_OBJ)

mpi: $(BIN_DIR) $(OUT_DIR) $(COMMON_OBJ) $(MPI_OBJ)
	$(MPICXX) $(CXXFLAGS) $(INCLUDES) -o $(BIN_DIR)/bow_mpi $(COMMON_OBJ) $(MPI_OBJ)

# Reglas genéricas para compilar a build/obj
$(OBJ_DIR)/src/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/serial/%.o: serial/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/mpi/%.o: mpi/%.cpp
	@mkdir -p $(dir $@)
	$(MPICXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)/bow_serial $(BIN_DIR)/bow_mpi
	rm -f src/**/*.o serial/*.o mpi/*.o src/*.o src/common/*.o src/utils/*.o
