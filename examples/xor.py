#!/usr/bin/python3
# -*- coding: utf-8 -*-

import tensorflow as tf
from tensorflow.keras import layers, models

trainingData = tf.constant([[0, 0], [0, 1], [1, 0], [1, 1]])
trainLabels  = tf.constant([    0,      1,      1,      0 ])

model = models.Sequential()
model.add(layers.Dense(32, input_dim=2, activation='relu'))
model.add(layers.Dense(1, activation='sigmoid'))

model.compile(loss='mean_squared_error',
              optimizer='adam',
              metrics=['binary_accuracy'])

model.fit(trainingData, trainLabels, epochs=400)
prediction = model.predict(trainingData)

for (a, b), (y,) in zip(trainingData, prediction):
  print("{:.0f} xor {:.0f} = {:.0f}".format(a, b, y))

  