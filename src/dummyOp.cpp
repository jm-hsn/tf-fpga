
#include "dummyOp.hpp"

namespace tf_lib {
  DummyOp::DummyOp(OpKernelConstruction* context) : AsyncOpKernel(context) {

  };

  void DummyOp::ComputeAsync(OpKernelContext* context, DoneCallback done) {
    // Input tensor is of the following dimensions:
    // [ batch, in_rows, in_cols, in_depth ]
    const Tensor& input = context->input(0);

    ///const int32 *p = input.flat<int32>().data();

    TensorShape input_shape = input.shape();

    TensorShape output_shape;
    const int32 dims[] = {dataLength};
    TensorShapeUtils::MakeShape(dims, 1, &output_shape);

    output_shape.set_dim(0, dims[0]);

    Tensor* output = nullptr;
    OP_REQUIRES_OK(context, context->allocate_output(0, output_shape, &output));

    auto input_tensor = input.tensor<int32, 1>();
    auto output_tensor = output->tensor<int32, 1>();

    auto worker = connectionManager.createWorker(Module::dummyModule, 1);
    {
      auto jobs = worker->getJobList();
      jobs->getJob(0)->setPayload(0, input_tensor(0));
    }
    worker->setDoneCallback([output_tensor, worker, done]{
      auto jobs = worker->getJobList();
      output_tensor(0) = jobs->getJob(0)->getResponsePayload(0);
      done();
    });

    worker->startSync();
  }
}