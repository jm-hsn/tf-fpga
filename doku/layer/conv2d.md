# Zweidimensionale Konvolution

## Tensorgröße

Input:
- in TF: `[batchSize, imageY, imageX, channels]`
- an FPGA: `[imageY, imageX]`

Kernel:
- in TF: `[kernelY, kernelX, channels, outputChannels]`
- an FPGA: `[kernelY, kernelX]`

Output:
- vom FPGA: `[imageY2, imageX2]`
- an TF: `[batchSize, imageY2, imageX2, outputChannels]`

## Parallelisierung

1.  **Ohne FPGA-seitigem Speicher**

    FPGA Recheneinheiten werden verteilt `(batchSize * channels * outputChannels)` Mal verwendet.

    ```python
    for sample in range(batchSize):
      for outputChannel in range(outputChannels):
        for channel in range(channels):
          output[sample][outputChannel] += f(
            input[sample][channel], 
            kernel[channel][outputChannel]
          )
    ```