CXX=/usr/bin/g++

SRC_DIR=src
INC_DIR=include
BUILD_DIR=build
TESTS_DIR=tests
FPGA_LIB_DIR=lib/mlfpga

CFLAGS=-g -Wall -pthread -std=c++11 -O3
LFLAGS=-shared -Wl,--no-as-needed,-Map=$(BUILD_DIR)/project.map


TF_CFLAGS=$(shell cat $(BUILD_DIR)/TF_CFLAGS)
TF_LFLAGS=$(shell cat $(BUILD_DIR)/TF_LFLAGS)

SRCS=$(wildcard $(SRC_DIR)/*.cpp)
OBJS=$(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

FPGA_LIB_SRCS=$(wildcard $(FPGA_LIB_DIR)/$(SRC_DIR)/*.cpp)
FPGA_LIB_OBJS=$(patsubst $(FPGA_LIB_DIR)/$(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(FPGA_LIB_SRCS))

TESTS_SRCS=$(wildcard $(TESTS_DIR)/main.cpp)
ECHO_SRCS=$(wildcard $(TESTS_DIR)/echo.cpp)

EXECUTABLE=op_lib.so
TEST_EXEC=test
ECHO_EXEC=echo

all: config $(BUILD_DIR)/$(EXECUTABLE)

config:
	@if [ ! -f "$(BUILD_DIR)/TF_CFLAGS" -o ! -f "$(BUILD_DIR)/TF_LFLAGS" ]; then ./configure $(BUILD_DIR) || exit 1; fi

$(BUILD_DIR)/$(EXECUTABLE): $(OBJS) $(FPGA_LIB_OBJS)
	$(CXX) $(LFLAGS) $(TF_LFLAGS) -o $@ $^

$(OBJS): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp $(INC_DIR)/%.hpp
	$(CXX) $(CFLAGS) -fPIC -c $(TF_CFLAGS) -I$(INC_DIR) -o $@ $<

$(FPGA_LIB_OBJS): $(BUILD_DIR)/%.o : $(FPGA_LIB_DIR)/$(SRC_DIR)/%.cpp $(FPGA_LIB_DIR)/$(INC_DIR)/%.hpp
	$(CXX) $(CFLAGS) -fPIC -c -I$(FPGA_LIB_DIR)/$(INC_DIR) -o $@ $<

test: config $(BUILD_DIR)/$(EXECUTABLE) $(BUILD_DIR)/$(TEST_EXEC)

$(BUILD_DIR)/$(TEST_EXEC): $(TESTS_SRCS) $(FPGA_LIB_OBJS)
	$(CXX) $(CFLAGS) -I$(FPGA_LIB_DIR)/$(INC_DIR) -o $@ $^

echo: $(BUILD_DIR)/$(ECHO_EXEC)

$(BUILD_DIR)/$(ECHO_EXEC): $(ECHO_SRCS)
	$(CXX) $(CFLAGS) -o $@ $^

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/$(EXECUTABLE) $(BUILD_DIR)/$(TEST_EXEC) $(BUILD_DIR)/$(ECHO_EXEC)