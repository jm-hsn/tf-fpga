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

    int outputSize = 28;

    void fpgaCall(const Tensor *input, const Tensor *kernel, Tensor *output, int sample, int channel, int filter);
    void delayThread(DoneCallback done);

  //TF_DISALLOW_COPY_AND_ASSIGN(Conv2DOp);
};