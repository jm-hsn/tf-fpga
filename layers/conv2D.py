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

    ch_inputs = tf.unstack(tf.dtypes.cast(inputs, dtype=tf.int32), axis=3)
    ch_kernel = tf.unstack(tf.dtypes.cast(self.kernel, dtype=tf.int32), axis=2)

    ch_outputs = [None] * len(ch_inputs)

    for ch in range(len(ch_inputs)):
      print(ch_inputs[ch], ch_kernel[ch])
      ch_outputs[ch] = [None] * self.filters
      kernel_2d = tf.unstack(ch_kernel[ch], axis=2)
      for f in range(len(kernel_2d)):
        ch_outputs[ch][f] = load_op.op_lib.MyConv2D(input=ch_inputs[ch], filter=kernel_2d[f])
      
      ch_outputs[ch] = tf.stack(ch_outputs[ch], axis=2)
    outs = tf.stack(ch_outputs, axis=2)
    return tf.dtypes.cast(outs, dtype=tf.float32)