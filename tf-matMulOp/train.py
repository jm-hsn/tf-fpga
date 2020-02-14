import tensorflow as tf
import tensorflow.keras as keras
from tensorflow.keras import layers
from tensorflow.keras.layers import Input, Embedding, LSTM, Dense, Dropout, Flatten, MaxPooling2D, Conv2D
from tensorflow.keras.models import Model, Sequential
from tensorflow.keras.datasets import mnist
from tensorflow.keras.utils import plot_model, to_categorical

import numpy as np
from IPython import embed

my_matmul_module = tf.load_op_library('./matMul.so')

batch_size = 128
num_classes = 10
epochs = 1 # 12

# input image dimensions
img_rows, img_cols = 28, 28

# the data, split between train and test sets
(x_train, y_train), (x_test, y_test) = mnist.load_data()

x_train = x_train.reshape(x_train.shape[0], img_rows, img_cols, 1)
x_test = x_test.reshape(x_test.shape[0], img_rows, img_cols, 1)
input_shape = (img_rows, img_cols, 1)

x_train = x_train.astype('float32')
x_test = x_test.astype('float32')
x_train /= 255
x_test /= 255
print('x_train shape:', x_train.shape)
print(x_train.shape[0], 'train samples')
print(x_test.shape[0], 'test samples')

# convert class vectors to binary class matrices
y_train = to_categorical(y_train, num_classes)
y_test = to_categorical(y_test, num_classes)

class Conv2DFPGA(layers.Layer):
  def __init__(self, kernel):
    super(Conv2DFPGA, self).__init__()
    self.kernel = kernel
  def call(self, inputs):
    ints = tf.dtypes.cast(inputs, dtype=tf.int32)
    outs = my_matmul_module.MyConv2D(input=ints, filter=ints)
    return tf.dtypes.cast(outs, dtype=tf.float32)

class MyConv2D(layers.Conv2D):

  def __init__(self,
               filters,
               kernel_size,
               strides=(1, 1),
               padding='valid',
               data_format=None,
               dilation_rate=(1, 1),
               activation=None,
               use_bias=True,
               kernel_initializer='glorot_uniform',
               bias_initializer='zeros',
               kernel_regularizer=None,
               bias_regularizer=None,
               activity_regularizer=None,
               kernel_constraint=None,
               bias_constraint=None,
               **kwargs):
    super(MyConv2D, self).__init__(
        filters=filters,
        kernel_size=kernel_size,
        strides=strides,
        padding=padding,
        data_format=data_format,
        dilation_rate=dilation_rate,
        activation=activation,
        use_bias=use_bias,
        kernel_initializer=kernel_initializer,
        bias_initializer=bias_initializer,
        kernel_regularizer=kernel_regularizer,
        bias_regularizer=bias_regularizer,
        activity_regularizer=activity_regularizer,
        kernel_constraint=kernel_constraint,
        bias_constraint=bias_constraint,
        **kwargs)
  def call(self, inputs):
      #inputs.get_shape(),
      #filter_shape=self.kernel.shape,
      #dilation_rate=self.dilation_rate,
      #strides=self.strides,
      #padding=self._padding_op,
      #data_format=self._conv_op_data_format)

      #kernel.shape.ndims
      #inputs.get_shape().ndims
    if self.rank == 1 and inputs.get_shape(): #fpga restriction
      return my_matmul_module.MyConv2D(inputs, self.kernel)
    else:
      return super(MyConv2D, self).call(inputs)

model = Sequential()
model.add(MyConv2D(32, kernel_size=(3, 3),
                 activation='relu',
                 input_shape=input_shape))
model.add(Conv2DFPGA([0,0]))
model.add(Flatten())
model.add(Dense(128, activation='relu'))
model.add(Dropout(0.5))
model.add(Dense(num_classes, activation='softmax'))

model.compile(loss=keras.losses.categorical_crossentropy,
              optimizer=keras.optimizers.Adadelta(),
              metrics=['accuracy'])
              
model.fit(x_train, y_train,
          batch_size=batch_size,
          epochs=epochs,
          verbose=1,
          validation_data=(x_test, y_test))

score = model.evaluate(x_test, y_test, verbose=0)
print('Test loss:', score[0])
print('Test accuracy:', score[1])

plot_model(model, to_file='model.png', expand_nested=True, show_shapes=True)