#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/function.h"
#include <stdlib.h>

#include <pthread.h>

using namespace tensorflow;
typedef FunctionDefHelper FDH;


class Conv2DOp : public AsyncOpKernel {
  public:
    explicit Conv2DOp(OpKernelConstruction* context);

    void ComputeAsync(OpKernelContext* context, DoneCallback done) override;

  private:
    int instance = -1;
  //TF_DISALLOW_COPY_AND_ASSIGN(Conv2DOp);
};