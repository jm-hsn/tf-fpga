CXX=/usr/bin/g++

SRC_DIR=src
INC_DIR=include
BUILD_DIR=build
FPGA_LIB_DIR=lib/mlfpga

CFLAGS=-g -Wall -pthread -std=c++11
LFLAGS=-shared -Wl,--no-as-needed,-Map=$(BUILD_DIR)/project.map


TF_CFLAGS=$(shell cat $(BUILD_DIR)/TF_CFLAGS)
TF_LFLAGS=$(shell cat $(BUILD_DIR)/TF_LFLAGS)

SRCS=$(wildcard $(SRC_DIR)/*.cpp)
OBJS=$(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

FPGA_LIB_SRCS=$(wildcard $(FPGA_LIB_DIR)/$(SRC_DIR)/*.cpp)
FPGA_LIB_OBJS=$(patsubst $(FPGA_LIB_DIR)/$(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(FPGA_LIB_SRCS))

EXECUTABLE=op_lib.so

all: config $(BUILD_DIR)/$(EXECUTABLE)

config:
	@if [ ! -f "$(BUILD_DIR)/TF_CFLAGS" -o ! -f "$(BUILD_DIR)/TF_LFLAGS" ]; then ./configure $(BUILD_DIR) || exit 1; fi

$(BUILD_DIR)/$(EXECUTABLE): $(OBJS) $(FPGA_LIB_OBJS)
	$(CXX) $(LFLAGS) $(TF_LFLAGS) -o $@ $^

$(OBJS): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp $(INC_DIR)/%.hpp
	$(CXX) $(CFLAGS) -fPIC -c $(TF_CFLAGS) -I$(INC_DIR) -o $@ $<

$(FPGA_LIB_OBJS): $(BUILD_DIR)/%.o : $(FPGA_LIB_DIR)/$(SRC_DIR)/%.cpp $(FPGA_LIB_DIR)/$(INC_DIR)/%.hpp
	$(CXX) $(CFLAGS) -fPIC -c -I$(FPGA_LIB_DIR)/$(INC_DIR) -o $@ $<

tf_cflags:

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/$(EXECUTABLE)