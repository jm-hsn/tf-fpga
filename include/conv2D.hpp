#ifndef CONV2D_FPGA_H
#define CONV2D_FPGA_H

#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/function.h"
#include <stdlib.h>

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <future>
#include <mutex>

#include "../lib/mlfpga/include/connectionManager.hpp"
#include "../lib/mlfpga/include/modules.hpp"

#include "entrypoint.hpp"

namespace tf_lib {

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
      int tagCounter = 0;

      int width = 224;
      int kernel = 5;
      int border = kernel/2;
      int sizeWithBorder = width + 2*border;
      int pixels = sizeWithBorder * sizeWithBorder;


    //TF_DISALLOW_COPY_AND_ASSIGN(Conv2DOp);
  };
}

#endif
