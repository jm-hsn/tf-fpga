
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/shape_inference.h"
#include "tensorflow/core/framework/function.h"
#include "tensorflow/core/util/tensor_format.h"

#include "tensorflow/core/lib/math/math_util.h"

#include "conv2D.hpp"

using namespace tensorflow;
using namespace tensorflow::shape_inference;
