/*
 *
 * CENG342 Project-3
 *
 * Downscaling SEQUENTIAL
 *
 * Usage:  main <input.jpg> <output.jpg> 
 *
 * @group_id 8
 * @author Emre Özçatal 20050111074, Semih Gür 19050111017, Emirhan Akıtürk 19050111065, Abdülsamet Haymana 19050111068
 *
 */
 
 #include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

#define CHANNEL_NUM 4


/*
    Donnscaling function, simply sum the color of each 4 pixel and make it one pixel. 
*/
__global__ void downscaleImage(const uint8_t *inputImage, int width, int height, uint8_t *downsampledImage)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    // To downscale it to half size 
    if (x < width / 2 && y < height / 2)
    {
        // Color defined in pixel as number up to 256
        int redSum = 0, greenSum = 0, blueSum = 0, alphaSum = 0;

        // Geting 4 pixel 
        for (int dy = 0; dy < 2; dy++)
        {
            for (int dx = 0; dx < 2; dx++)
            {
                int pixelIndex = ((y * 2 + dy) * width + (x * 2 + dx)) * CHANNEL_NUM;
                redSum += inputImage[pixelIndex];
                greenSum += inputImage[pixelIndex + 1];
                blueSum += inputImage[pixelIndex + 2];
                alphaSum += inputImage[pixelIndex + 3];
            }
        }

        // We will sum it to one pixel 
        int downsampledPixelIndex = (y * width / 2 + x) * CHANNEL_NUM;
        downsampledImage[downsampledPixelIndex] = redSum / 4;
        downsampledImage[downsampledPixelIndex + 1] = greenSum / 4;
        downsampledImage[downsampledPixelIndex + 2] = blueSum / 4;
        downsampledImage[downsampledPixelIndex + 3] = alphaSum / 4;
    }
}

void performCUDAImageDownscaling(const uint8_t *inputImage, int width, int height, uint8_t *downsampledImage)
{
    uint8_t *d_inputImage, *d_downsampledImage;
    size_t inputSize = width * height * CHANNEL_NUM * sizeof(uint8_t);
    size_t downsampledSize = (width / 2) * (height / 2) * CHANNEL_NUM * sizeof(uint8_t);

    // Allocate device memory
    cudaMalloc((void **)&d_inputImage, inputSize);
    cudaMalloc((void **)&d_downsampledImage, downsampledSize);

    // Copy input image data to device memory
    cudaMemcpy(d_inputImage, inputImage, inputSize, cudaMemcpyHostToDevice);

    // Set grid and block dimensions
    dim3 blockDim(16, 16);
    dim3 gridDim((width / 2 + blockDim.x - 1) / blockDim.x, (height / 2 + blockDim.y - 1) / blockDim.y);

    // Launch the kernel
    downscaleImage<<<gridDim, blockDim>>>(d_inputImage, width, height, d_downsampledImage);

    // Copy the result back to host memory
    cudaMemcpy(downsampledImage, d_downsampledImage, downsampledSize, cudaMemcpyDeviceToHost);

    // Free device memory
    cudaFree(d_inputImage);
    cudaFree(d_downsampledImage);
}

int main(int argc, char *argv[])
{
    clock_t start_time, end_time;
    double execution_time;

    int originalWidth, originalHeight, bpp;
    uint8_t *inputImage = stbi_load(argv[1], &originalWidth, &originalHeight, &bpp, CHANNEL_NUM);

    printf("Original Width: %d  Height: %d \n", originalWidth, originalHeight);
    printf("Input: %s , Output: %s  \n", argv[1], argv[2]);

    // Start measuring time
    start_time = clock(); 

    int downsampledWidth = originalWidth / 2;
    int downsampledHeight = originalHeight / 2;
    uint8_t *downsampledImage = (uint8_t *)malloc(downsampledWidth * downsampledHeight * CHANNEL_NUM * sizeof(uint8_t));

    performCUDAImageDownscaling(inputImage, originalWidth, originalHeight, downsampledImage);

    stbi_write_jpg(argv[2], downsampledWidth, downsampledHeight, CHANNEL_NUM, downsampledImage, 100);

    stbi_image_free(inputImage);
    free(downsampledImage);

    // Stop measuring time
    end_time = clock(); 

    execution_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC * 1000;

    printf("Execution time: %.2f milliseconds\n", execution_time);
    return 0;
}
