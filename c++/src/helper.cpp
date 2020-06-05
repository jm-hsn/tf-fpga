#include "helper.hpp"

namespace tf_lib {

  using namespace tensorflow;
  using namespace tensorflow::shape_inference;


  Status DimensionsFromShape(ShapeHandle shape, TensorFormat format,
                            DimensionHandle* batch_dim,
                            gtl::MutableArraySlice<DimensionHandle> spatial_dims,
                            DimensionHandle* filter_dim,
                            InferenceContext* context) {
    const int32 rank = GetTensorDimsFromSpatialDims(spatial_dims.size(), format);
    // Batch.
    *batch_dim = context->Dim(shape, GetTensorBatchDimIndex(rank, format));
    // Spatial.
    for (uint spatial_dim_index = 0; spatial_dim_index < spatial_dims.size();
        ++spatial_dim_index) {
      spatial_dims[spatial_dim_index] = context->Dim(
          shape, GetTensorSpatialDimIndex(rank, format, spatial_dim_index));
    }
    // Channel.
    *filter_dim = context->Dim(shape, GetTensorFeatureDimIndex(rank, format));
    if (format == FORMAT_NCHW_VECT_C) {
      TF_RETURN_IF_ERROR(context->Multiply(
          *filter_dim,
          context->Dim(shape, GetTensorInnerFeatureDimIndex(rank, format)),
          filter_dim));
    }
    return Status::OK();
  }

  Status ShapeFromDimensions(DimensionHandle batch_dim,
                            gtl::ArraySlice<DimensionHandle> spatial_dims,
                            DimensionHandle filter_dim, TensorFormat format,
                            InferenceContext* context, ShapeHandle* shape) {
    const int32 rank = GetTensorDimsFromSpatialDims(spatial_dims.size(), format);
    std::vector<DimensionHandle> out_dims(rank);

    // Batch.
    out_dims[tensorflow::GetTensorBatchDimIndex(rank, format)] = batch_dim;
    // Spatial.
    for (uint spatial_dim_index = 0; spatial_dim_index < spatial_dims.size();
        ++spatial_dim_index) {
      out_dims[tensorflow::GetTensorSpatialDimIndex(
          rank, format, spatial_dim_index)] = spatial_dims[spatial_dim_index];
    }
    // Channel.
    if (format == tensorflow::FORMAT_NCHW_VECT_C) {
      // When format is NCHW_VECT_C, factor the feature map count
      // into the outer feature count and the inner feature count (=4).
      TF_RETURN_IF_ERROR(context->Divide(
          filter_dim, 4, /*evenly_divisible=*/true,
          &out_dims[tensorflow::GetTensorFeatureDimIndex(rank, format)]));
      out_dims[GetTensorInnerFeatureDimIndex(rank, format)] = context->MakeDim(4);
    } else {
      out_dims[tensorflow::GetTensorFeatureDimIndex(rank, format)] = filter_dim;
    }

    *shape = context->MakeShape(out_dims);
    return tensorflow::Status::OK();
  }

}