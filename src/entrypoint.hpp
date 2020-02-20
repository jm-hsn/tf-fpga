
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/shape_inference.h"
#include "tensorflow/core/framework/function.h"

#include "tensorflow/core/lib/math/math_util.h"

#include "conv2D.hpp"



REGISTER_OP("MyConv2D")
    .Input("input: int32")
    .Input("filter: int32")
    .Output("output: int32")
    .SetShapeFn([](::tensorflow::shape_inference::InferenceContext* c) {
      c->set_output(0, c->input(0));
      return Status::OK();
    });

REGISTER_KERNEL_BUILDER(Name("MyConv2D").Device(DEVICE_CPU), Conv2DOp);