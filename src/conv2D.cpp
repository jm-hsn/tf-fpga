#ifndef CONV2D_FPGA
#define CONV2D_FPGA

#include "conv2D.hpp"

volatile int instances = 0;
volatile int inParallel = 0;
std::mutex printMu;

void Conv2DOp::delayThread(DoneCallback done) {
  printMu.lock();
  //printf("parallel: %2d instance: %2d '%s' %dms sleep\n", ++inParallel, instance, name().c_str(), delay);
  printMu.unlock();
  std::this_thread::sleep_for(milliseconds(delay));
  printMu.lock();
  //printf("parallel: %2d instance: %2d '%s' done\n", --inParallel, instance, name().c_str());
  printMu.unlock();
  done();
}

int width = 224;
int kernel = 5;
int border = kernel/2;
int sizeWithBorder = width + 2*border;
int pixels = sizeWithBorder * sizeWithBorder;
int tagCounter = 0;

void Conv2DOp::fpgaCall(const Tensor *input, const Tensor *kernel, Tensor *output, int sample, int channel, int filter) {
    auto input_tensor = input->tensor<int32, 4>();
    auto kernel_tensor = kernel->tensor<int32, 4>();
    auto output_tensor = output->tensor<int32, 4>();
    
    jobData job(pixels);
    *job.moduleId = moduleIds[conv2D_5x5_Module];
    job.tag = tagCounter++;

    for(int x=0; x<outputSize; x++) {
      for(int y=0; y<outputSize; y++) {
        job.payload[x*outputSize + y] = input_tensor(sample, x, y, channel);
      }
    }

    sendJob(&job, pixels);

    printMu.lock();
    /*
    printf(" sample: %3d, channel: %3d, filter: %3d\n", sample, channel, filter);
    
    for(int x=0; x<outputSize; x++) {
      for(int y=0; y<outputSize; y++) {
        printf("%c", input_tensor(sample, x, y, channel) > 0 ? '#' : ' ');
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
    */
    printMu.unlock();
}

Conv2DOp::Conv2DOp(OpKernelConstruction* context) : AsyncOpKernel(context) {
  instance = instances++;
  OP_REQUIRES_OK(context, context->GetAttr("delay", &delay));

};

void Conv2DOp::ComputeAsync(OpKernelContext* context, DoneCallback done) {
  // Input tensor is of the following dimensions:
  // [ batch, in_rows, in_cols, in_depth ]
  const Tensor& input = context->input(0);

  ///const int32 *p = input.flat<int32>().data();

  // Input filter is of the following dimensions:
  // [ filter_rows, filter_cols, in_depth, out_depth]
  const Tensor& kernel = context->input(1);

  TensorShape kernel_shape = kernel.shape();
  TensorShape input_shape = input.shape();


  int batchSize = input_shape.dim_size(0);
  int channels = input_shape.dim_size(3);
  int filters = kernel_shape.dim_size(3);

  TensorShape output_shape;
  const int32 dims[] = {batchSize, outputSize, outputSize, channels * filters};
  TensorShapeUtils::MakeShape(dims, 4, &output_shape);

  output_shape.set_dim(0, batchSize);
  output_shape.set_dim(1, outputSize);
  output_shape.set_dim(2, outputSize);
  output_shape.set_dim(3, channels * filters);

  //printMu.lock();
  //std::cout << output_shape.DebugString() << std::endl;
  //printMu.unlock();

  // Output tensor is of the following dimensions:
  // [ in_batch, out_rows, out_cols, out_depth ]
  Tensor* output = nullptr;
  OP_REQUIRES_OK(context, context->allocate_output(0, output_shape, &output));

  for(int sample=0; sample<batchSize; sample++) {
    for(int channel=0; channel<channels; channel++) {
      for(int filter=0; filter<filters; filter++) {
        std::async(std::launch::async, &Conv2DOp::fpgaCall, this, &input, &kernel, output, sample, channel, filter);
        
      }
    }
  }
  std::async(std::launch::async, &Conv2DOp::delayThread, this, done);
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
  const string opname = "MyMatMul";
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

#endif
