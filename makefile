CXX=/usr/bin/g++

CFLAGS=-g -Wall -pthread -std=c++11
LFLAGS=-shared -Wl,--no-as-needed

TF_CFLAGS=$(shell python3 -c 'import tensorflow as tf; print(" ".join(tf.sysconfig.get_compile_flags()))' 2>/dev/null)
TF_LFLAGS=$(shell python3 -c 'import tensorflow as tf; print(" ".join(tf.sysconfig.get_link_flags()))' 2>/dev/null)

SRC_DIR=./src
INC_DIR=./src
BUILD_DIR=./build

SRCS=$(wildcard $(SRC_DIR)/*.cpp)
OBJS=$(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

EXECUTABLE=op_lib.so

all: dir $(BUILD_DIR)/$(EXECUTABLE)

dir:
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(EXECUTABLE): $(OBJS)
	$(CXX) $(LFLAGS) $(TF_LFLAGS) -o $@ $^

$(OBJS): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp $(INC_DIR)/%.hpp
	$(CXX) $(CFLAGS) -fPIC -c $(TF_CFLAGS) -I$(INC_DIR) -o $@ $< -O2

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/$(EXECUTABLE)