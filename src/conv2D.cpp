#ifndef CONV2D_FPGA
#define CONV2D_FPGA

#include "conv2D.hpp"

volatile int instances = 0;
volatile int inParallel = 0;
std::mutex mu;

void delayThread(int ins, const char *name, int delay, std::function<void ()> done) {
  mu.lock();
  printf("parallel: %2d instance: %2d '%s' %dms sleep\n", ++inParallel, ins, name, delay);
  mu.unlock();
  std::this_thread::sleep_for(milliseconds(delay));
  mu.lock();
  printf("parallel: %2d instance: %2d '%s' done\n", --inParallel, ins, name);
  mu.unlock();
  done();
}

Conv2DOp::Conv2DOp(OpKernelConstruction* context) : AsyncOpKernel(context) {
  instance = instances++;
  OP_REQUIRES_OK(context, context->GetAttr("delay", &delay));

};

void Conv2DOp::ComputeAsync(OpKernelContext* context, DoneCallback done) {
  // Input tensor is of the following dimensions:
  // [ batch, in_rows, in_cols, in_depth ]
  const Tensor& input = context->input(0);

  // Input filter is of the following dimensions:
  // [ filter_rows, filter_cols, in_depth, out_depth]
  const Tensor& filter = context->input(1);
  TensorShape filterShape = filter.shape();

  TensorShape out_shape = input.shape();

  // Output tensor is of the following dimensions:
  // [ in_batch, out_rows, out_cols, out_depth ]
  Tensor* output = nullptr;
  OP_REQUIRES_OK(context, context->allocate_output(0, out_shape, &output));

  context->cancellation_manager();

  std::async(std::launch::async, delayThread, instance, name().c_str(), delay, done);
  
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
