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
    input = [0,1,2,3]
    with self.session():
      result = load_op.op_lib.MyDummy(input=input)
      self.assertAllEqual(result, input)

if __name__ == "__main__":
  tf.test.main()
