import tensorflow as tf
import tensorflow.keras as keras
from tensorflow.keras import layers
from tensorflow.keras.layers import Input, Embedding, LSTM, Dense, Dropout, Flatten, MaxPooling2D, Conv2D
from tensorflow.keras.models import Model, Sequential
from tensorflow.keras.datasets import mnist
from tensorflow.keras.utils import plot_model, to_categorical

import numpy as np
from IPython import embed

import sys
sys.path.append('..')
from hostLib.layers.conv2D import Conv2D as Conv2DFPGA

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

x_train = x_train.astype('float')
x_test = x_test.astype('float')
#x_train /= 255
#x_test /= 255
print('x_train shape:', x_train.shape)
print(x_train.shape[0], 'train samples')
print(x_test.shape[0], 'test samples')

# convert class vectors to binary class matrices
y_train = to_categorical(y_train, num_classes)
y_test = to_categorical(y_test, num_classes)

a = layers.Input(dtype=tf.int32, shape=(28, 28, 1))
b = Conv2DFPGA(2)(a)
c = Conv2DFPGA(1)(a)
d = Conv2DFPGA(1)(b)
e = Conv2DFPGA(2)(c)

print(a)
print(b)
print(c)
print(d)
print(e)

x = layers.Add()([c,c])
y = layers.Flatten()(x)
z = layers.Dense(num_classes, activation='softmax')(y)

model = Model(inputs=a, outputs=z)
print(model.output_shape)

"""
model = Sequential()
model.add(Conv2DFPGA([0,0]))
model.add(Conv2DFPGA([0,0]))
model.add(Conv2DFPGA([0,0]))
model.add(Flatten())
model.add(Dense(128, activation='relu'))
model.add(Dropout(0.5))
model.add(Dense(num_classes, activation='softmax'))
"""
model.compile(loss=keras.losses.categorical_crossentropy,
              optimizer=keras.optimizers.Adadelta(),
              metrics=['accuracy'])

plot_model(model, to_file='model.png', expand_nested=True, show_shapes=True)

model.fit(x_train, y_train,
          batch_size=batch_size,
          epochs=epochs,
          verbose=1,
          validation_data=(x_test, y_test))

score = model.evaluate(x_test, y_test, verbose=0)
print('Test loss:', score[0])
print('Test accuracy:', score[1])

