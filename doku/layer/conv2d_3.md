# 2D convolution with activation and fixed output channels

## TF equivalent

```python
layers.Conv2d(
  filters=32,
  kernel_size=5,
  strides=(1, 1),
  padding='valid',
  data_format='channels_last',
  dilation_rate=(1, 1),
  activation='relu',
  use_bias=False,
  trainable=True
)
```

## Tensor sizes

Input:
- from TF: `[batchSize, imageY, imageX, channels]`
- to FPGA: `[imageY, imageX, channels]`

Filter:
- from TF: `[kernelY, kernelX, channels, outputChannels]`
- to FPGA: `[kernelY, kernelX, channels, outputChannels]`

Output:
- from FPGA: `[imageY2, imageX2, outputChannels]`
- to TF: `[batchSize, imageY2, imageX2, outputChannels]`

## Parallelization

- `(batchSize)` jobs will be created.
- layer can be called in parallel (functional model)

```python
for sample in range(batchSize):
    output[sample] = job(
      input[sample], 
      kernel
    )
```

## Constraints

```python
  imageX  = imageY  = 228
  kernelX = kernelY = 5
  imageX2 = imageY2 = 224
  channels = 3
  outputChannels = 32
```

## Job lengths

- to FPGA:
  `5 * 5 * 3 * 32 + 228 * 228 * 3 = 158352`
- from FPGA:
  `224 * 224 * 32 = 1605632`

## Gradient
  same as layers.Conv2d: `conv2d_backprop` in 
  `tensorflow/tensorflow/python/ops/nn_ops.py:2290`