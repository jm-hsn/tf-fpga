import tensorflow as tf
zero_out_module = tf.load_op_library('./zero_out.so')

print(zero_out_module.zero_out([[1, 2], [3, 4]]))

# Prints
# array([[1, 0], [0, 0]], dtype=int32)
