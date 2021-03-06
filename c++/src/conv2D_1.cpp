
#include "conv2D_1.hpp"

namespace tf_lib {

  volatile int instances = 0;
  volatile int inParallel = 0;
  std::mutex printMu;

  ShapeFunction conv2d_shape_fn = [](InferenceContext* c) {
    //INPUT:  NHWC
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
      /*
    DimensionHandle filter_rows_dim = c->Dim(
      filter_shape, GetFilterDimIndex<num_spatial_dims>(filter_format, 'H'));
    DimensionHandle filter_cols_dim = c->Dim(
      filter_shape, GetFilterDimIndex<num_spatial_dims>(filter_format, 'W'));
      
    DimensionHandle filter_input_depth_dim = c->Dim(
      filter_shape, GetFilterDimIndex<num_spatial_dims>(filter_format, 'I'));
      */
    DimensionHandle output_rows, output_cols, output_channels;
    c->Subtract(input_spatial_dims[0], 4, &output_rows);
    c->Subtract(input_spatial_dims[1], 4, &output_cols);

    c->Subtract(output_depth_dim, 0, &output_channels);

    std::vector<DimensionHandle> out_dims(4);
    out_dims[0] = batch_size_dim;
    out_dims[1] = output_rows;
    out_dims[2] = output_cols;
    out_dims[3] = output_channels;

    output_shape = c->MakeShape(out_dims);
    c->set_output(0, output_shape);
    return Status::OK();
  };

  Conv2DOp::Conv2DOp(OpKernelConstruction* context) : AsyncOpKernel(context) {
    instance = instances++;
  };

  void Conv2DOp::ComputeAsync(OpKernelContext* context, DoneCallback done) {
    init();
    // ############ TensorFlow namespace #############

    // Input tensor is of the following dimensions:
    // [ batch, in_rows, in_cols, in_depth ]
    const Tensor& input = context->input(0);

    // Input filter is of the following dimensions:
    // [ filter_rows, filter_cols, in_depth, out_depth]
    const Tensor& kernel = context->input(1);

    TensorShape kernel_shape = kernel.shape();
    TensorShape input_shape = input.shape();

    OP_REQUIRES_ASYNC(context, input_shape.dim_size(1) == 228, errors::InvalidArgument("Unsupported input height: ", input_shape.dim_size(1)), done);
    OP_REQUIRES_ASYNC(context, input_shape.dim_size(2) == 228, errors::InvalidArgument("Unsupported input width: ", input_shape.dim_size(2)), done);
    OP_REQUIRES_ASYNC(context, kernel_shape.dim_size(0) == 5, errors::InvalidArgument("Unsupported kernel height: ", kernel_shape.dim_size(0)), done);
    OP_REQUIRES_ASYNC(context, kernel_shape.dim_size(1) == 5, errors::InvalidArgument("Unsupported kernel width: ", kernel_shape.dim_size(1)), done);
    OP_REQUIRES_ASYNC(context, kernel_shape.dim_size(2) == input_shape.dim_size(3), 
      errors::InvalidArgument("kernel channels != input channels: ", kernel_shape.dim_size(2), " != ", input_shape.dim_size(3)), done);

    int batchSize = input_shape.dim_size(0);
    int channels = input_shape.dim_size(3);
    int outputChannels = kernel_shape.dim_size(3);

    // create output tensor
    TensorShape output_shape;
    const int32 dims[] = {batchSize, outputSize, outputSize, outputChannels};
    TensorShapeUtils::MakeShape(dims, 4, &output_shape);

    output_shape.set_dim(0, batchSize);
    output_shape.set_dim(1, outputSize);
    output_shape.set_dim(2, outputSize);
    output_shape.set_dim(3, outputChannels);

    // Output tensor is of the following dimensions:
    // [ in_batch, out_rows, out_cols, out_depth ]
    Tensor* output = nullptr;
    OP_REQUIRES_OK(context, context->allocate_output(0, output_shape, &output));

    // get data references
    auto input_tensor = input.tensor<float, 4>();
    auto kernel_tensor = kernel.tensor<float, 4>();
    auto output_tensor = output->tensor<float, 4>();

    // ############ FPGA communications library #############
    auto worker = connectionManager.createWorker(Module::conv2D_5x5_Module, batchSize * channels * outputChannels);
    {
      worker->setJobTimeout(milliseconds(300));
      worker->setRetryCount(10);
      auto jobs = worker->getJobList();

      for(int sample=0; sample<batchSize; sample++) {
        for(int outputChannel=0; outputChannel<outputChannels; outputChannel++) {
          for(int channel=0; channel<channels; channel++) {
            // get each job
            auto job = jobs->getJob(sample * outputChannels * channels + outputChannel * channels + channel);
            
            // write kernel to job
            for(int x=0; x<kernelSize; x++) {
              for(int y=0; y<kernelSize; y++) {
                job->setPayload(y*kernelSize + x, *((uint32_t*)&kernel_tensor(y, x, channel, outputChannel)));
              }
            }
            // write input pixels to job
            for(int x=0; x<sizeWithBorder; x++) {
              for(int y=0; y<sizeWithBorder; y++) {
                job->setPayload(kernelSize*kernelSize + y*sizeWithBorder + x, *((uint32_t*)&input_tensor(sample, y, x, channel)));
              }
            }
            job->setReady();
          }
        }
      }
    }
    worker->setDoneCallback([output_tensor, worker, done, batchSize, channels, outputChannels, this]{
      auto jobs = worker->getJobList();
      for(int sample=0; sample<batchSize; sample++) {
        for(int outputChannel=0; outputChannel<outputChannels; outputChannel++) {
          //set output matrix to zero 
          for(int x=0; x<outputSize; x++) {
            for(int y=0; y<outputSize; y++) {
              output_tensor(sample, y, x, outputChannel) = 0;
            }
          }
          //accumulate the pixels of all output channels
          for(int channel=0; channel<channels; channel++) {
            auto job = jobs->getJob(sample * outputChannels * channels + outputChannel * channels + channel);
            for(int x=0; x<outputSize; x++) {
              for(int y=0; y<outputSize; y++) {
                uint32_t pixel = job->getResponsePayload((y+border*2)*sizeWithBorder + (x+border*2));
                output_tensor(sample, y, x, outputChannel) += *((float*)&pixel);
              }
            }
          }
        }
      }
      done();
      connectionManager.removeFinishedWorkers();
    });
    worker->startAsync();

  }


  static Status MatMulGradHelper(FunctionDef* g, const string& opname,
                                const string& attr_adj_x,
                                const string& attr_adj_y, const string& x0,
                                bool ax0, const string& x1, bool ax1,
                                const string& y0, bool ay0, const string& y1,
                                bool ay1) {
    // The final outputs are "dx" and "dy". If we're broadcasting compute
    // intermediate nodes for now.
    std::vector<FDH::Node> nodes = {
        {{("dx")},
        opname,
        {x0, x1},
        {{"T", "$T"}, {attr_adj_x, ax0}, {attr_adj_y, ax1}}},
        {{("dy")},
        opname,
        {y0, y1},
        {{"T", "$T"}, {attr_adj_x, ay0}, {attr_adj_y, ay1}}},
    };

    *g = FDH::Define(
        // Arg defs
        {"x: T", "y: T", "dz: T"},
        // Ret val defs
        {"dx: T", "dy: T"},
        // Attr defs
        {{"T: {half, float, double}"}},
        // Nodes
        nodes);
    return Status::OK();
  }

  Status MatMulGrad(const AttrSlice& attrs, FunctionDef* g) {
    const string opname = "MyConv2D";
    const string attr_adj_x = "transpose_a";
    const string attr_adj_y = "transpose_b";
    DataType T;
    TF_RETURN_IF_ERROR(GetNodeAttr(attrs, "T", &T));
    if (T == DT_COMPLEX64 || T == DT_COMPLEX128) {
      return errors::Unimplemented(
          "MatMul gradient for complex is not supported yet.");
    }
    bool ta;
    bool tb;
    TF_RETURN_IF_ERROR(GetNodeAttr(attrs, attr_adj_x, &ta));
    TF_RETURN_IF_ERROR(GetNodeAttr(attrs, attr_adj_y, &tb));

    if (!ta && !tb) {
      return MatMulGradHelper(g, opname, attr_adj_x, attr_adj_y, "dz", false, "y",
                              true, "x", true, "dz", false);
    }
    if (!ta && tb) {
      return MatMulGradHelper(g, opname, attr_adj_x, attr_adj_y, "dz", false, "y",
                              false, "dz", true, "x", false);
    }
    if (ta && !tb) {
      return MatMulGradHelper(g, opname, attr_adj_x, attr_adj_y, "y", false, "dz",
                              true, "x", false, "dz", false);
    }
    CHECK(ta && tb);
    return MatMulGradHelper(g, opname, attr_adj_x, attr_adj_y, "y", true, "dz",
                            true, "dz", true, "x", true);
  }

  REGISTER_OP("MyConv2D_1")
      .Input("input: float")
      .Input("filter: float")
      .Output("output: float")
      .SetShapeFn(conv2d_shape_fn);

  REGISTER_KERNEL_BUILDER(Name("MyConv2D_1").Device(DEVICE_CPU), Conv2DOp);
  REGISTER_OP_GRADIENT("MyConv2D_1", MatMulGrad);

}