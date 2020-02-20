import os 
import tensorflow as tf

dir_path = os.path.dirname(os.path.realpath(__file__))
op_lib = tf.load_op_library(dir_path + '/build/op_lib.so')