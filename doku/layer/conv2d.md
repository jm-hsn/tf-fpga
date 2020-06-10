# Zweidimensionale Konvolution

## Tensorgröße

Input:
- in TF: `[batchSize, imageY, imageX, channels]`
- an FPGA: `[imageY, imageX]`

Kernel:
- in TF: `[kernelY, kernelX, channels, filters]`
- an FPGA: `[kernelY, kernelX]`

Output:
- vom FPGA: `[imageY2, imageX2]`
- an TF: `[batchSize, imageY2, imageX2, channels * filters]`

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