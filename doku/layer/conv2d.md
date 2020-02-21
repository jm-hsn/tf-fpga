# Zweidimensionale Konvolution

## Tensorgröße

Input:
- in TF: `[batchSize, imageX, imageY, channels]`
- an FPGA: `[imageX, imageY]`

Kernel:
- in TF: `[kernelX, kernelY, channels, filters]`
- an FPGA: `[kernelX, kernelY]`

Output:
- vom FPGA: `[imageX2, imageY2]`
- an TF: `[batchSize, imageX2, imageY2, channels * filters]`

## Parallelisierung

1.  **Ohne FPGA-seitigem Speicher**

    FPGA Recheneinheiten werden verteilt `(batchSize * channels * filters)` Mal verwendet.

    ```python
    for sample in range(batchSize):
      for channnel in range(channels):
        for filter in range(filters):
          output[sample][channel + filter * channels] = f(
            input[sample][channel], 
            kernel[channel][filter]
          )
    ```