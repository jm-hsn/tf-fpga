
#ifndef HELPER_FPGA_H
#define HELPER_FPGA_H 

#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/shape_inference.h"
#include "tensorflow/core/framework/function.h"
#include "tensorflow/core/util/tensor_format.h"

#include "tensorflow/core/lib/math/math_util.h"

namespace tf_lib {


  using namespace tensorflow;
  using namespace tensorflow::shape_inference;
  
  typedef Status (*ShapeFunction)(InferenceContext*);

  Status DimensionsFromShape(ShapeHandle shape, TensorFormat format,
                            DimensionHandle* batch_dim,
                            gtl::MutableArraySlice<DimensionHandle> spatial_dims,
                            DimensionHandle* filter_dim,
                            InferenceContext* context);

  Status ShapeFromDimensions(DimensionHandle batch_dim,
                            gtl::ArraySlice<DimensionHandle> spatial_dims,
                            DimensionHandle filter_dim, TensorFormat format,
                            InferenceContext* context, ShapeHandle* shape);
}
#endif