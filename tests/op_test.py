import tensorflow as tf
from tensorflow import nn
import numpy as np
from IPython import embed

import sys
sys.path.append('../hostLib/')
from hostLib.layers.conv2d import Conv2D as Conv2DFPGA
from hostLib import load_op


class FPGALibTest(tf.test.TestCase):
  def testDummyOp(self):
    input = [1337, 42, 2**31-1, -1]
    with self.session():
      result = load_op.op_lib.MyDummy(input=input)
      self.assertAllEqual(result, input)

  def testDummyOp100(self):
    with self.session():
      for i in range(100):
        input = [i+1, 42, 2**31-1, -1]
        result = load_op.op_lib.MyDummy(input=input)
        self.assertAllEqual(result, input)

  def testDummyOpBig(self):
    input = [i+1 for i in range(1024)]
    with self.session():
      result = load_op.op_lib.MyDummyBig(input=input)
      self.assertAllEqual(result, input)

  def testDummyOpBig100(self):
    with self.session():
      for i in range(100):
        input = [i*100+k+1 for k in range(1024)]
        result = load_op.op_lib.MyDummyBig(input=input)
        self.assertAllEqual(result, input)

  def testConv2DSingle(self):
    img = np.ndarray((228,228), dtype=float)
    img.fill(0)
    for a in range(228):
      for b in range(228):
        img[a][b] = (a)*228+(b)
    kernel = np.ndarray((5,5), dtype=float)
    kernel.fill(0)
    kernel[2][2] = 1
    input = tf.constant(np.expand_dims(img, (0, 3)), dtype=float)
    filter = tf.constant(np.expand_dims(kernel, (2, 3)), dtype=float)
    with self.session():
      ref = nn.convolution(input, filter)
      result = load_op.op_lib.MyConv2D(input=input, filter=filter)
      self.assertAllClose(result, ref)

  def testConv2DRandom(self):
    input = tf.random.uniform(shape=[1,228,228,1])
    filter = tf.random.uniform(shape=[5,5,1,1])
    with self.session():
      ref = nn.convolution(input, filter)
      result = load_op.op_lib.MyConv2D(input=input, filter=filter)
      self.assertAllClose(result, ref)

  def testConv2DMulti(self):
    input = tf.random.uniform(shape=[2,228,228,3])
    filter = tf.random.uniform(shape=[5,5,3,4])
    with self.session():
      ref = nn.convolution(input, filter)
      result = load_op.op_lib.MyConv2D(input=input, filter=filter)
      self.assertAllClose(result, ref)

if __name__ == "__main__":
  tf.test.main()
