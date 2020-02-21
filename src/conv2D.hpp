#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/function.h"
#include <stdlib.h>

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <future>
#include <mutex>

using namespace tensorflow;
using namespace std::chrono;
typedef FunctionDefHelper FDH;


class Conv2DOp : public AsyncOpKernel {
  public:
    explicit Conv2DOp(OpKernelConstruction* context);

    void ComputeAsync(OpKernelContext* context, DoneCallback done) override;

  private:

    int instance = -1;
    int delay = 1000;

  //TF_DISALLOW_COPY_AND_ASSIGN(Conv2DOp);
};