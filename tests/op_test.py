import tensorflow as tf
import numpy as np
from IPython import embed

import sys
import os
sys.path.append('..')
from hostLib.layers.conv2D import Conv2D as Conv2DFPGA
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

if __name__ == "__main__":
  tf.test.main()
