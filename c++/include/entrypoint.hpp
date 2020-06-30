#ifndef ENTRY_FPGA_H
#define ENTRY_FPGA_H

#include <fstream>
#include "json.hpp"

#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/shape_inference.h"
#include "tensorflow/core/framework/function.h"
#include "tensorflow/core/util/tensor_format.h"

#include "tensorflow/core/lib/math/math_util.h"

#include "conv2D_1.hpp"
#include "conv2D_2.hpp"
#include "conv2D_3.hpp"
#include "conv2D_maxpool.hpp"
#include "conv2D_maxpool_multi.hpp"
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
