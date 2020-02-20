import tensorflow as tf
from tensorflow.keras import layers

from .. import load_op

class Conv2D(layers.Layer):
  def __init__(self, kernel):
    super(Conv2D, self).__init__()
    self.kernel = kernel
  def call(self, inputs):
    ints = tf.dtypes.cast(inputs, dtype=tf.int32)
    outs = load_op.op_lib.MyConv2D(input=ints, filter=ints)
    return tf.dtypes.cast(outs, dtype=tf.float32)