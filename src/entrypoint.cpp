
#include "entrypoint.hpp"

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
    for (int spatial_dim_index = 0; spatial_dim_index < spatial_dims.size();
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
    for (int spatial_dim_index = 0; spatial_dim_index < spatial_dims.size();
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

  REGISTER_OP("MyConv2D")
      .Input("input: float")
      .Input("filter: float")
      .Output("output: float")
      .SetShapeFn([](InferenceContext* c) {
        //INPUT: NHWC
        //KERNEL: HWIO
        //OUTPUT: NHWC

        constexpr int num_spatial_dims = 2;
        TensorFormat data_format;
        FormatFromString("NHWC", &data_format);
        FilterTensorFormat filter_format;
        FilterFormatFromString("HWIO", &filter_format);

        ShapeHandle input_shape, filter_shape, output_shape;
        TF_RETURN_IF_ERROR(c->WithRank(c->input(0), 4, &input_shape));
        TF_RETURN_IF_ERROR(c->WithRank(c->input(1), 4, &filter_shape));

        DimensionHandle batch_size_dim;
        DimensionHandle input_depth_dim;
        gtl::InlinedVector<DimensionHandle, 2> input_spatial_dims(2);
        TF_RETURN_IF_ERROR(DimensionsFromShape(
          input_shape, data_format, &batch_size_dim,
          absl::MakeSpan(input_spatial_dims), &input_depth_dim, c));

        DimensionHandle output_depth_dim = c->Dim(
          filter_shape, GetFilterDimIndex<num_spatial_dims>(filter_format, 'O'));
        DimensionHandle filter_rows_dim = c->Dim(
          filter_shape, GetFilterDimIndex<num_spatial_dims>(filter_format, 'H'));
        DimensionHandle filter_cols_dim = c->Dim(
          filter_shape, GetFilterDimIndex<num_spatial_dims>(filter_format, 'W'));
        DimensionHandle filter_input_depth_dim = c->Dim(
          filter_shape, GetFilterDimIndex<num_spatial_dims>(filter_format, 'I'));

        DimensionHandle output_rows, output_cols, output_channels;
        c->Add(input_spatial_dims[0], 0, &output_rows);
        c->Add(input_spatial_dims[1], 0, &output_cols);

        c->Multiply(filter_input_depth_dim, output_depth_dim, &output_channels);

        std::vector<DimensionHandle> out_dims(4);
        out_dims[0] = batch_size_dim;
        out_dims[1] = output_rows;
        out_dims[2] = output_cols;
        out_dims[3] = output_channels;

        output_shape = c->MakeShape(out_dims);
        c->set_output(0, output_shape);
        return Status::OK();
      });

  REGISTER_KERNEL_BUILDER(Name("MyConv2D").Device(DEVICE_CPU), Conv2DOp);

  REGISTER_OP("MyDummy")
    .Input("input: int32")
    .Output("output: int32")
    .SetShapeFn([](::tensorflow::shape_inference::InferenceContext* c) {
      c->set_output(0, c->input(0));
      return Status::OK();
    });
  ;

  REGISTER_KERNEL_BUILDER(Name("MyDummy").Device(DEVICE_CPU), DummyOp);

  REGISTER_OP("MyDummyBig")
    .Input("input: int32")
    .Output("output: int32")
    .SetShapeFn([](::tensorflow::shape_inference::InferenceContext* c) {
      c->set_output(0, c->input(0));
      return Status::OK();
    });
  ;

  REGISTER_KERNEL_BUILDER(Name("MyDummyBig").Device(DEVICE_CPU), DummyBigOp);

  ConnectionManager connectionManager;

  void __attribute__ ((constructor)) init(void) {
    printf("fpga library loaded\n");
  }

}