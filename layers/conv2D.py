import tensorflow as tf
from tensorflow.python.framework import tensor_shape
from tensorflow.keras import layers, initializers, regularizers, constraints

from .. import load_op

class Conv2D(layers.Layer):
  def __init__(self,
    filters = 1,
    kernel_initializer = 'glorot_uniform',
               kernel_regularizer=None,
               kernel_constraint=None,
    ):
    super(Conv2D, self).__init__()
    #int, dim of output space
    self.filters = filters
    self.kernel_initializer = initializers.get(kernel_initializer)
    self.kernel_regularizer = regularizers.get(kernel_regularizer)
    self.kernel_constraint = constraints.get(kernel_constraint)


  def build(self, input_shape):
    input_shape = tf.TensorShape(input_shape)
    self.input_channel = input_shape[3]
    kernel_shape = (5,)*2 + (self.input_channel, self.filters)

    self.kernel = self.add_weight(
        name='kernel',
        shape=kernel_shape,
        initializer=self.kernel_initializer,
        regularizer=self.kernel_regularizer,
        constraint=self.kernel_constraint,
        trainable=True,
        dtype=self.dtype)

  def call(self, inputs):

    #out = tf.Tensor(tf.int32, shape=inputs.shape)
    intKernel = tf.cast(self.kernel, dtype=tf.int32)
    return load_op.op_lib.MyConv2D(input=inputs, filter=intKernel, delay=100*self.filters)