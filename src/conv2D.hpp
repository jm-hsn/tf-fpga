#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/function.h"

using namespace tensorflow;
typedef FunctionDefHelper FDH;


class Conv2DOp : public OpKernel {
 public:
  explicit Conv2DOp(OpKernelConstruction* context) : OpKernel(context) {};

  void Compute(OpKernelContext* context) override;

  //TF_DISALLOW_COPY_AND_ASSIGN(Conv2DOp);
};