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
#include "helper.hpp"

namespace tf_lib {

  using namespace tensorflow;
  using namespace std::chrono;
  typedef FunctionDefHelper FDH;

  extern ShapeFunction conv2d_shape_fn;

  class Conv2DOp : public AsyncOpKernel {
    public:
      explicit Conv2DOp(OpKernelConstruction* context);

      void ComputeAsync(OpKernelContext* context, DoneCallback done) override;

    private:

      int instance = -1;
      
      int tagCounter = 0;

      int width = 224;
      int kernelSize = 5;
      int border = kernelSize/2;
      int sizeWithBorder = width + 2*border;
      int pixels = sizeWithBorder * sizeWithBorder;
      int outputSize = width;


    //TF_DISALLOW_COPY_AND_ASSIGN(Conv2DOp);
  };
}

#endif
