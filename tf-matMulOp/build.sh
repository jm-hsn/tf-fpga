#!/bin/bash

if [ "$TF_CFLAGS" == "" ]; then
  export TF_CFLAGS=( $(python3 -c 'import tensorflow as tf; print(" ".join(tf.sysconfig.get_compile_flags()))' 2>/dev/null) )
fi
if [ "$TF_LFLAGS" == "" ]; then
  export TF_LFLAGS=( $(python3 -c 'import tensorflow as tf; print(" ".join(tf.sysconfig.get_link_flags()))' 2>/dev/null) )
fi

g++ -g -std=c++11 -shared matMul.cc -o matMul.so -fPIC ${TF_CFLAGS[@]} ${TF_LFLAGS[@]} -O2 -Wall
