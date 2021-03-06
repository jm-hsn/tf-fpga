
#include "dummyOp.hpp"

namespace tf_lib {
  DummyOp::DummyOp(OpKernelConstruction* context) : AsyncOpKernel(context) {

  };

  void DummyOp::ComputeAsync(OpKernelContext* context, DoneCallback done) {
    init();
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

    auto worker = connectionManager.createWorker(Module::dummyModule);
    worker->setJobTimeout(milliseconds(100));
    worker->setRetryCount(10);
    {
      auto job = worker->getJobList()->getJob(0);

      for(size_t i=0; i<job->getPayloadSize(); i++) {
        job->setPayload(i, input_tensor(i));
        job->setReady();
      }
    }
    worker->setDoneCallback([output_tensor, worker, done]{
      auto job = worker->getJobList()->getJob(0);
      for(size_t i=0; i<job->getResponsePayloadSize(); i++) {
        output_tensor(i) = job->getResponsePayload(i);
      }
      done();
      connectionManager.removeFinishedWorkers();
    });

    worker->startAsync();
  }
}