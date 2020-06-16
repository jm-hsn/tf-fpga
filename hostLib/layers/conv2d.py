import tensorflow as tf
from tensorflow.python.ops import nn_ops
from tensorflow.python.framework import ops
from tensorflow.python.ops import array_ops
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

    return load_op.op_lib.MyConv2D(input=inputs, filter=self.kernel)

@ops.RegisterGradient("MyConv2D")
def _my_conv_2d_grad(op, grad):
  shape_0, shape_1 = array_ops.shape_n([op.inputs[0], op.inputs[1]])

  return [
      nn_ops.conv2d_backprop_input(
          shape_0,
          op.inputs[1],
          grad,
          strides=[1,1,1,1],
          padding="VALID"
          ),
      nn_ops.conv2d_backprop_filter(
          op.inputs[0],
          shape_1,
          grad,
          strides=[1,1,1,1],
          padding="VALID"
          )
  ]