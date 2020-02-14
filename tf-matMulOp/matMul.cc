#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/shape_inference.h"
#include "tensorflow/core/framework/function.h"

#include "tensorflow/core/lib/math/math_util.h"

using namespace tensorflow;
typedef FunctionDefHelper FDH;

REGISTER_OP("MyMatMul")
    .Input("to_zero: int32")
    .Output("zeroed: int32")
    .SetShapeFn([](::tensorflow::shape_inference::InferenceContext* c) {
      c->set_output(0, c->input(0));
      return Status::OK();
    });

REGISTER_OP("MyConv2D")
    .Input("input: int32")
    .Input("filter: int32")
    .Output("output: int32")
    .SetShapeFn([](::tensorflow::shape_inference::InferenceContext* c) {
      c->set_output(0, c->input(0));
      return Status::OK();
    });
    
#include "tensorflow/core/framework/op_kernel.h"

using namespace tensorflow;
/*
class Conv2DOp : public OpKernel {
 public:
  explicit Conv2DOp(OpKernelConstruction* context) : OpKernel(context) {}

  void Compute(OpKernelContext* context) override {
    // Grab the input tensor
    const Tensor& input_tensor = context->input(0);
    auto input = input_tensor.flat<int32>();

    printf("call n: %d\n", n++);

    // Create an output tensor
    Tensor* output_tensor = NULL;
    OP_REQUIRES_OK(context, context->allocate_output(0, input_tensor.shape(),
                                                     &output_tensor));
    auto output_flat = output_tensor->flat<int32>();

    // Set all but the first element of the output tensor to 0.
    const int N = input.size();
    
    for (int i = 1; i < N; i++) {
      output_flat(i) = 0;
    }
    // Preserve the first input value if possible.
    if (N > 0) output_flat(0) = input(0);
  }

  int n = 0;
};
*/


class Conv2DOp : public OpKernel {
 public:
  explicit Conv2DOp(OpKernelConstruction* context) : OpKernel(context) {
  }

  void Compute(OpKernelContext* context) override {
    // Input tensor is of the following dimensions:
    // [ batch, in_rows, in_cols, in_depth ]
    const Tensor& input = context->input(0);

    // Input filter is of the following dimensions:
    // [ filter_rows, filter_cols, in_depth, out_depth]
    const Tensor& filter = context->input(1);

    TensorShape out_shape = input.shape();

    // Output tensor is of the following dimensions:
    // [ in_batch, out_rows, out_cols, out_depth ]
    Tensor* output = nullptr;
    OP_REQUIRES_OK(context, context->allocate_output(0, out_shape, &output));

    std::cout << "Conv2D" << std::endl;

    // If there is nothing to compute, return.
    if (out_shape.num_elements() == 0) {
      return;
    }

    
  }

 private:
  //LaunchConv2DOp<Device, T> launcher_;

  TF_DISALLOW_COPY_AND_ASSIGN(Conv2DOp);
};


REGISTER_KERNEL_BUILDER(Name("MyConv2D").Device(DEVICE_CPU), Conv2DOp);

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

REGISTER_OP_GRADIENT("MyConv2D", MatMulGrad);