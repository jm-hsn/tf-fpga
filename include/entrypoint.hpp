#ifndef ENTRY_FPGA_H
#define ENTRY_FPGA_H

#include <fstream>
#include "../lib/json/single_include/nlohmann/json.hpp"

#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/shape_inference.h"
#include "tensorflow/core/framework/function.h"
#include "tensorflow/core/util/tensor_format.h"

#include "tensorflow/core/lib/math/math_util.h"

#include "conv2D.hpp"
#include "dummyOp.hpp"
#include "dummyBigOp.hpp"
#include "../lib/mlfpga/include/connectionManager.hpp"

#include "helper.hpp"

namespace tf_lib {
  void __attribute__ ((constructor)) construct(void);

  extern ConnectionManager connectionManager;
  void init();
}
#endif
