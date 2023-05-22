/**
 *
 * CENG342 Project-1
 *
 * Downscaling SEQUENTIAL
 *
 * Usage:  main <input.jpg> <output.jpg> 
 *
 * @group_id 8
 * @author Emre Özçatal 20050111074, Semih Gür 19050111017, Emirhan Akıtürk 19050111065, Abdülsamet Haymana 19050111068
 *
 * @version 2.17, 30 April 2023
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <sched.h>
#include <hwloc.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

#define CHANNEL_NUM 4

void performParallelDownscaling(uint8_t* inputRGBImage, int originalWidth, int originalHeight, uint8_t* downsampledImage, int rank, int size);
long getCurrentCoreFrequency();

int main(int argc, char* argv[])
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int originalWidth, originalHeight, bpp;

    uint8_t* inputImage = stbi_load(argv[1], &originalWidth, &originalHeight, &bpp, CHANNEL_NUM);

    if (rank == 0) {
        printf("Original Width: %d  Height: %d \n", originalWidth, originalHeight);
        printf("Input: %s , Output: %s  \n", argv[1], argv[2]);
    }

    double startTime = MPI_Wtime();

    int downsampledWidth = originalWidth / 2;
    int downsampledHeight = originalHeight / 2;

    uint8_t* downsampledImage = (uint8_t*)malloc(downsampledWidth * downsampledHeight * CHANNEL_NUM * sizeof(uint8_t));
    performParallelDownscaling(inputImage, originalWidth, originalHeight, downsampledImage, rank, size);

    double endTime = MPI_Wtime();

    if (rank == 0) {
        printf("Elapsed time: %lf \n", endTime - startTime);

        stbi_write_jpg(argv[2], downsampledWidth, downsampledHeight, CHANNEL_NUM, downsampledImage, 100);
    }

    stbi_image_free(inputImage);
    stbi_image_free(downsampledImage);

    MPI_Finalize();
    return 0;
}

void performParallelDownscaling(uint8_t* rgbImage, int width, int height, uint8_t* downsampledImage, int rank, int size)
{
    int downsampledWidth = width / 2;
    int downsampledHeight = height / 2;

    long* cpuFrequencies = (long*)malloc(size * sizeof(long));
    long myFrequency = getCurrentCoreFrequency();
    MPI_Allgather(&myFrequency, 1, MPI_LONG, cpuFrequencies, 1, MPI_LONG, MPI_COMM_WORLD);

    long totalFrequency = 0;
    for (int i = 0; i < size; i++) {
        totalFrequency += cpuFrequencies[i];
    }

    int pixelsPerProcess = (downsampledWidth * downsampledHeight * cpuFrequencies[rank]) / totalFrequency;
    int startIndex = rank * pixelsPerProcess;
    int endIndex = startIndex + pixelsPerProcess;
    if (endIndex > downsampledWidth * downsampledHeight) {
        endIndex = downsampledWidth * downsampledHeight;
    }

    #pragma omp parallel for collapse(2) schedule(dynamic) num_threads(4)
    for (int y = startIndex / downsampledWidth; y < endIndex / downsampledWidth; y++) {
        for (int x = 0; x < downsampledWidth; x++) {
            int redSum = 0, greenSum = 0, blueSum = 0, alphaSum = 0;

            for (int dy = 0; dy < 2; dy++) {
                for (int dx = 0; dx < 2; dx++) {
                    int pixelIndex = ((y * 2 + dy) * width + (x * 2 + dx)) * CHANNEL_NUM;
                    redSum += rgbImage[pixelIndex];
                    greenSum += rgbImage[pixelIndex + 1];
                    blueSum += rgbImage[pixelIndex + 2];
                    alphaSum += rgbImage[pixelIndex + 3];
                }
            }

            int downsampledPixelIndex = (y * downsampledWidth + x) * CHANNEL_NUM;
            downsampledImage[downsampledPixelIndex] = redSum / 4;
            downsampledImage[downsampledPixelIndex + 1] = greenSum / 4;
            downsampledImage[downsampledPixelIndex + 2] = blueSum / 4;
            downsampledImage[downsampledPixelIndex + 3] = alphaSum / 4;
        }
    }

    int* recvCounts = (int*)malloc(size * sizeof(int));
    int* displacements = (int*)malloc(size * sizeof(int));

    for (int i = 0; i < size; i++) {
        int startIdx = i * pixelsPerProcess;
        int endIdx = startIdx + pixelsPerProcess;
        if (endIdx > downsampledWidth * downsampledHeight) {
            endIdx = downsampledWidth * downsampledHeight;
        }
        recvCounts[i] = (endIdx - startIdx) * CHANNEL_NUM;
        displacements[i] = startIdx * CHANNEL_NUM;
    }

    MPI_Allgatherv(MPI_IN_PLACE, pixelsPerProcess * CHANNEL_NUM, MPI_UNSIGNED_CHAR,
        downsampledImage, recvCounts, displacements, MPI_UNSIGNED_CHAR, MPI_COMM_WORLD);

    free(recvCounts);
    free(displacements);
    free(cpuFrequencies);
}

long getCurrentCoreFrequency()
{
    FILE* cpuInfo = fopen("/proc/cpuinfo", "rb");
    if (cpuInfo == NULL) {
        perror("Failed to open /proc/cpuinfo");
        return -1;
    }

    long frequency = -1;
    char line[256];
    while (fgets(line, sizeof(line), cpuInfo)) {
        if (sscanf(line, "cpu MHz : %ld", &frequency) == 1) {
            break;
        }
    }

    fclose(cpuInfo);

    return frequency;
}