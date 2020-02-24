CXX=/usr/bin/g++

CFLAGS=-g -Wall -pthread -std=c++11
LFLAGS=-shared -Wl,--no-as-needed

SRC_DIR=./src
INC_DIR=./src
BUILD_DIR=./build

TF_CFLAGS=$(shell cat $(BUILD_DIR)/TF_CFLAGS)
TF_LFLAGS=$(shell cat $(BUILD_DIR)/TF_LFLAGS)

SRCS=$(wildcard $(SRC_DIR)/*.cpp)
OBJS=$(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

EXECUTABLE=op_lib.so

all: config $(BUILD_DIR)/$(EXECUTABLE)

config:
	@if [ ! -f "$(BUILD_DIR)/TF_CFLAGS" -o ! -f "$(BUILD_DIR)/TF_LFLAGS" ]; then ./configure $(BUILD_DIR) || exit 1; fi

$(BUILD_DIR)/$(EXECUTABLE): $(OBJS)
	$(CXX) $(LFLAGS) $(TF_LFLAGS) -o $@ $^

$(OBJS): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp $(INC_DIR)/%.hpp
	$(CXX) $(CFLAGS) -fPIC -c $(TF_CFLAGS) -I$(INC_DIR) -o $@ $<

tf_cflags:

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/$(EXECUTABLE)