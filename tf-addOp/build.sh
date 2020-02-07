#!/bin/bash

if [ "$TF_CFLAGS" == "" ]; then
  export TF_CFLAGS=( $(python3 -c 'import tensorflow as tf; print(" ".join(tf.sysconfig.get_compile_flags()))') )
fi
if [ "$TF_LFLAGS" == "" ]; then
  export TF_LFLAGS=( $(python3 -c 'import tensorflow as tf; print(" ".join(tf.sysconfig.get_link_flags()))') )
fi

g++ -std=c++11 -shared zero_out.cc -o zero_out.so -fPIC ${TF_CFLAGS[@]} ${TF_LFLAGS[@]} -O2 -v

#g++ -g -std=c++11 -shared zero_out.cc -o zero_out.so -fPIC -I/usr/local/lib/python3.6/dist-packages/tensorflow_core/include -D_GLIBCXX_USE_CXX11_ABI=0 -L/usr/local/lib/python3.6/dist-packages/tensorflow_core -l:libtensorflow_framework.so.2 -O2

#g++ -g -std=c++11 -shared zero_out.cc -o zero_out.so -fPIC ${TF_CFLAGS[@]} ${TF_LFLAGS[@]} -O2 -Wall -Wl,-z,defs

