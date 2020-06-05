#!/usr/local/bin/python
# -*- coding: utf-8 -*-

import numpy as np
import cv2
from mss import mss
from PIL import Image
import threading
import time
from random import randint

import tensorflow as tf
from tensorflow.keras import layers, models

import sys
sys.path.append('../hostLib/')
from hostLib.layers.conv2d import Conv2D as Conv2DFPGA

bounding_box = {'top': 0, 'left': 0, 'width': 2560, 'height': 1440}
width, height = 228, 228

sct = mss()
stop = 0

a = layers.Input(dtype=tf.float32, shape=(width, height, 3))
z = Conv2DFPGA(1)(a)
model = models.Model(inputs=a, outputs=z)


model.compile(loss=tf.keras.losses.categorical_crossentropy,
              optimizer=tf.keras.optimizers.Adadelta(),
              metrics=['accuracy'])

sct_img = sct.grab(bounding_box)
np_img = np.array(sct_img)
resized_image = cv2.resize(np_img, (width, height))
resized_image = cv2.cvtColor(resized_image, cv2.COLOR_BGRA2BGR)
while True:
  cv2.ellipse(
    resized_image, 
    (randint(0,width), randint(0,height)), 
    (randint(0,width/2), randint(0,height/2)), 
    randint(0,360), 
    0, 
    360, 
    [randint(0,256), randint(0,256), randint(0,256)],
    10
  )
  img32 = tf.cast(resized_image, tf.float32)
  #img32 = np.expand_dims(img32, axis=2)

  cv2.imshow('screen', resized_image)
  x,y,w,h = cv2.getWindowImageRect('screen')
  batch = np.expand_dims(img32, axis=0)
  batch = tf.tile(batch, [1,1,1,1])

  print(batch.shape)

  predictions = model.predict(batch)


  pred8 = tf.cast(predictions / 256, tf.uint8)
  for i in range(pred8.shape[0]):
    name = 'conv_{}'.format(i)
    cv2.imshow(name, pred8.numpy()[i])
    cv2.moveWindow(name, x+((i+1)*300)%1500, y+int((i+1)/5)*300)


  if (cv2.waitKey(1) & 0xFF) == ord('x') or stop:
    cv2.destroyAllWindows()
    stop = True
    break