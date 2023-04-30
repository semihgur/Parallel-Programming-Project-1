/**
 *
 * CENG342 Project-1
 *
 * Downscaling PARALLEL
 *
 * Usage:  main <input.jpg> <output.jpg> 
 *
 * @group_id 8
 * @author Emre Özçatal 20050111074, Semih Gür 19050111017, Emirhan Akıtürk 19050111065, Abdülsamet Haymana
 *
 * @version 2.17, 30 April 2023
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

#define CHANNEL_NUM 4

void parallel_downscaling(uint8_t* rgb_image, int width, int height, uint8_t* downsampled_image, int rank, int size);

int main(int argc, char* argv[]) 
{   
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int width, height, bpp;

    uint8_t* input_image = stbi_load(argv[1], &width, &height, &bpp, CHANNEL_NUM);

    if (rank == 0) {
        printf("Width: %d  Height: %d \n", width, height);
        printf("Input: %s , Output: %s  \n", argv[1], argv[2]);
    }

    double time1 = MPI_Wtime();   

    uint8_t* downsampled_image = (uint8_t*)malloc(width/2 * height/2 * CHANNEL_NUM * sizeof(uint8_t));
    parallel_downscaling(input_image, width, height, downsampled_image, rank, size);

    double time2 = MPI_Wtime();
    if (rank == 0) {
        printf("Elapsed time: %lf \n", time2 - time1);    

        stbi_write_jpg(argv[2], width/2, height/2, CHANNEL_NUM, downsampled_image, 100);
    }

    stbi_image_free(input_image);
    stbi_image_free(downsampled_image);

    MPI_Finalize();
    return 0;
}

void parallel_downscaling(uint8_t* rgb_image, int width, int height, uint8_t* downsampled_image, int rank, int size) {
    //Calculating downsampled image size
    int downsampled_width = width / 2;
    int downsampled_height = height / 2;
    //Calculating the number of pixels per process
    int pixels_per_process = (downsampled_width * downsampled_height + size - 1) / size;
    int start_index = rank * pixels_per_process;
    int end_index = start_index + pixels_per_process;
    if (end_index > downsampled_width * downsampled_height) {
        end_index = downsampled_width * downsampled_height;
    }

    //Downsampling the image
    for (int i = start_index; i < end_index; i++) {
        //Calculating the x and y coordinates of the pixel in the downsampled image
        int x = i % downsampled_width;
        int y = i / downsampled_width;

        int r = 0, g = 0, b = 0, a = 0;

        //Calculating the average of the 4 pixels in the original image
        for (int dy = 0; dy < 2; dy++) {
            for (int dx = 0; dx < 2; dx++) {
                int pixel_index = ((y * 2 + dy) * width + (x * 2 + dx)) * 4;

                r += rgb_image[pixel_index];
                g += rgb_image[pixel_index + 1];
                b += rgb_image[pixel_index + 2];
                a += rgb_image[pixel_index + 3];
            }
        }
        //Calculating the index of the pixel in the downsampled image
        int downsampled_pixel_index = (y * downsampled_width + x) * 4;
        //Calculating the average of the 4 pixels in the original image
        downsampled_image[downsampled_pixel_index] = r / 4;
        downsampled_image[downsampled_pixel_index + 1] = g / 4;
        downsampled_image[downsampled_pixel_index + 2] = b / 4;
        downsampled_image[downsampled_pixel_index + 3] = a / 4;
    }

    //Gathering the downsampled image from all processes
    //We are creating recvcounts and displs arrays for MPI_Allgatherv so that we can get different number of pixels from each process
    //recvcounts[i] is the number of pixels that process i will send
    int* recvcounts = (int*) malloc(size * sizeof(int));
    //displs[i] is the index of the first pixel that process i will send
    int* displs = (int*) malloc(size * sizeof(int));
    //We are calculating the number of pixels that each process will send and the index of the first pixel that each process will send
    for (int i = 0; i < size; i++) {
        int start_i = i * pixels_per_process;
        int end_i = start_i + pixels_per_process;
        if (end_i > downsampled_width * downsampled_height) {
            end_i = downsampled_width * downsampled_height;
        }
        recvcounts[i] = (end_i - start_i) * CHANNEL_NUM;
        displs[i] = start_i * CHANNEL_NUM;
    }
    //We are gathering the downsampled image from all processes
    MPI_Allgatherv(MPI_IN_PLACE, pixels_per_process * CHANNEL_NUM, MPI_UNSIGNED_CHAR,
                   downsampled_image, recvcounts, displs, MPI_UNSIGNED_CHAR,
                   MPI_COMM_WORLD);

    free(recvcounts);
    free(displs);
}