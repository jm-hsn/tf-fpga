import tensorflow as tf
from tensorflow import nn
import numpy as np
import time

import sys
sys.path.append('../hostLib/')
from hostLib.layers.conv2d import Conv2D as Conv2DFPGA
from hostLib import load_op

def run(inputShape, filterShape, n):

  input = tf.random.uniform(shape=inputShape)
  filter = tf.random.uniform(shape=filterShape)

  start = time.time()
  for i in range(n):
    nn.convolution(input, filter)
  elapsed_time = time.time() - start
  print("shapes: {:22s} {:22s}, count: {:6d},  CPU Conv2D OP time: {:.6f} s".format(str(inputShape), str(filterShape), n, elapsed_time))

  start = time.time()
  for i in range(n):
    load_op.op_lib.MyConv2D_1(input=input, filter=filter)
  elapsed_time = time.time() - start
  print("shapes: {:22s} {:22s}, count: {:6d}, FPGA Conv2D OP time: {:.6f} s".format(str(inputShape), str(filterShape), n, elapsed_time))

input = tf.random.uniform(shape=[1,228,228,1])
filter = tf.random.uniform(shape=[5,5,1,1])
nn.convolution(input, filter)
load_op.op_lib.MyConv2D_1(input=input, filter=filter)


run((1,228,228,1), (5,5,1,1), 10000)
run((10,228,228,1), (5,5,1,1), 1000)
run((100,228,228,1), (5,5,1,1), 100)
run((1000,228,228,1), (5,5,1,1), 10)


run((1,228,228,1), (5,5,1,1), 10000)
run((1,228,228,10), (5,5,10,1), 1000)
run((1,228,228,100), (5,5,100,1), 100)
run((1,228,228,1000), (5,5,1000,1), 10)


run((1,228,228,1), (5,5,1,1), 10000)
run((1,228,228,1), (5,5,1,10), 1000)
run((1,228,228,1), (5,5,1,100), 100)
run((1,228,228,1), (5,5,1,1000), 10)

run((1,228,228,1), (5,5,1,1000), 10)
run((1,228,228,10), (5,5,10,100), 10)
run((1,228,228,100), (5,5,100,10), 10)
run((1,228,228,1000), (5,5,1000,1), 10)