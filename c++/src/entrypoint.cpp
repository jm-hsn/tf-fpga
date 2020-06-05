
#include "entrypoint.hpp"

namespace tf_lib {

  using namespace tensorflow;
  using namespace tensorflow::shape_inference;

  REGISTER_OP("MyConv2D")
      .Input("input: float")
      .Input("filter: float")
      .Output("output: float")
      .SetShapeFn(conv2d_shape_fn);

  REGISTER_KERNEL_BUILDER(Name("MyConv2D").Device(DEVICE_CPU), Conv2DOp);

  REGISTER_OP("MyDummy")
    .Input("input: int32")
    .Output("output: int32")
    .SetShapeFn([](InferenceContext* c) {
      c->set_output(0, c->input(0));
      return Status::OK();
    });
  ;

  REGISTER_KERNEL_BUILDER(Name("MyDummy").Device(DEVICE_CPU), DummyOp);

  REGISTER_OP("MyDummyBig")
    .Input("input: int32")
    .Output("output: int32")
    .SetShapeFn([](InferenceContext* c) {
      c->set_output(0, c->input(0));
      return Status::OK();
    });
  ;

  REGISTER_KERNEL_BUILDER(Name("MyDummyBig").Device(DEVICE_CPU), DummyBigOp);

  ConnectionManager connectionManager;

  bool hasInitialized = false;

  void init() {
    if(hasInitialized)
      return;

    std::ifstream configStream("config.json");
    nlohmann::json config;
    configStream >> config;

    auto fpgas = config["fpgas"];

    for(uint i=0; i<fpgas.size(); i++) {
      string ip = fpgas[i]["ip"];
      const uint port = fpgas[i]["port"];
      connectionManager.addFPGA(ip.c_str(), port);
      printf("added fpga %u at %s:%u\n", i, ip.c_str(), port);
    }

    connectionManager.start();

    printf("fpga server started\n");
    hasInitialized = true;
  }

  void __attribute__ ((constructor)) construct(void) {
    printf("fpga library loaded\n");
  }

}