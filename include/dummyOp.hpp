#ifndef DUMMY_OP_FPGA_H
#define DUMMY_OP_FPGA_H

#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/function.h"
#include <stdlib.h>

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>

#include "../lib/mlfpga/include/connectionManager.hpp"
#include "../lib/mlfpga/include/modules.hpp"

#include "entrypoint.hpp"

namespace tf_lib {
  using namespace tensorflow;
  using namespace std::chrono;

  class DummyOp : public AsyncOpKernel {
    public:
      explicit DummyOp(OpKernelConstruction* context);
      void ComputeAsync(OpKernelContext* context, DoneCallback done) override;

    private:
      void fpgaCall(const Tensor *input, Tensor *output, DoneCallback done);

      const int dataLength = 4;
      int tagCounter = 0;

  };
}
#endif
