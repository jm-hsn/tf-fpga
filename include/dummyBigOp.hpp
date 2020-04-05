#ifndef DUMMY_BIG_OP_FPGA_H
#define DUMMY_BIG_OP_FPGA_H

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

  class DummyBigOp : public AsyncOpKernel {
    public:
      explicit DummyBigOp(OpKernelConstruction* context);
      void ComputeAsync(OpKernelContext* context, DoneCallback done) override;

    private:
      const int dataLength = 1024;
      int tagCounter = 0;

  };
}
#endif
