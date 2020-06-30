# multiple 2D convolutions with MaxPooling

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

layers.MaxPooling2D(
  pool_size=(224, 224),
  strides=(2, 2),
  padding='valid',
  data_format='channels_last'
)

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

layers.MaxPooling2D(
  pool_size=(108, 108),
  strides=(2, 2),
  padding='valid',
  data_format='channels_last'
)

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

layers.MaxPooling2D(
  pool_size=(50, 50),
  strides=(2, 2),
  padding='valid',
  data_format='channels_last'
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
  imageX2 = imageY2 = 25
  channels = 3
  outputChannels = 32
```

## Job lengths

- to FPGA:
  `5 * 5 * (3 * 32 + 32 * 32 * 2) + 228 * 228 * 3 = 209552`
- from FPGA:
  `25 * 25 * 32 = 20000`

## Gradient
  tbd