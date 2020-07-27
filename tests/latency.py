import tensorflow as tf
from tensorflow import nn
import tensorflow.keras as keras
from tensorflow.keras import layers
from tensorflow.keras.models import Model, Sequential
import numpy as np
import time

import sys
sys.path.append('../hostLib/')
from hostLib.layers.conv2d import Conv2D as Conv2DFPGA
from hostLib import load_op

def run(inputShape, filterShape, n):

  input = tf.random.uniform(shape=inputShape)
  filter = tf.random.uniform(shape=filterShape)

  for i in range(n):
    start = time.time()
    nn.convolution(input, filter)
    elapsed_time = time.time() - start
    print("CPU Conv2D OP: {:.6f} s".format(elapsed_time))

  for i in range(n):
    start = time.time()
    load_op.op_lib.MyConv2D_1(input=input, filter=filter)
    elapsed_time = time.time() - start
    print("FPGA Conv2D OP: {:.6f} s".format(elapsed_time))

input = tf.random.uniform(shape=[1,228,228,1])
filter = tf.random.uniform(shape=[5,5,1,1])
nn.convolution(input, filter)
load_op.op_lib.MyConv2D_1(input=input, filter=filter)


#run((1,228,228,1), (5,5,1,1), 1000)

def runLayer(inputShape, n):

  input_x = tf.random.uniform(shape=inputShape)
  input_y = tf.image.resize(input_x, (224,224))
  
  model = Sequential()
  model.add(layers.Conv2D(
    1,
    kernel_size=5,
    strides=(1, 1),
    padding='valid',
    data_format='channels_last',
    dilation_rate=(1, 1),
    activation=None,
    use_bias=False,
    trainable=True
  ))
  model.compile(loss=keras.losses.categorical_crossentropy,
              optimizer=keras.optimizers.Adadelta())

  for i in range(n):
    start = time.time()
    model.evaluate(input_x, input_y, verbose=0)
    elapsed_time = time.time() - start
    print("CPU Conv2D layer: {:.6f} s".format(elapsed_time))

  model = Sequential()
  model.add(Conv2DFPGA(1))
  model.compile(loss=keras.losses.categorical_crossentropy,
              optimizer=keras.optimizers.Adadelta())

  for i in range(n):
    start = time.time()
    model.evaluate(input_x, input_y, verbose=0)
    elapsed_time = time.time() - start
    print("FPGA Conv2D layer: {:.6f} s".format(elapsed_time))

runLayer((1,228,228,1), 1000)