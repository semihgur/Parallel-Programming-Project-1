#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
//Defining the number of channels
#define CHANNEL_NUM 4

void paralel_downscaling(uint8_t* rgb_image, int width, int height, uint8_t* downsampled_image, int rank, int size);

int main(int argc, char* argv[]) 
{   
    // Initialize MPI
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int width, height, bpp;
    // Load image
    uint8_t* input_image = stbi_load(argv[1], &width, &height, &bpp, CHANNEL_NUM);

    // Print image info
    if (rank == 0) {
        printf("Width: %d  Height: %d \n", width, height);
        printf("Input: %s , Output: %s  \n", argv[1], argv[2]);
    }

    // Start timer
    double time1 = MPI_Wtime();   

    // Allocate memory for output image
    uint8_t* downsampled_image = (uint8_t*)malloc(width/2 * height/2 * CHANNEL_NUM * sizeof(uint8_t));
    // Downscale image
    paralel_downscaling(input_image, width, height, downsampled_image, rank, size);

    // Stop timer
    double time2 = MPI_Wtime();
    if (rank == 0) {
        // Print elapsed time
        printf("Elapsed time: %lf \n", time2 - time1);    
        // Save image
        stbi_write_jpg(argv[2], width/2, height/2, CHANNEL_NUM, downsampled_image, 100);
    }
    // Free memory
    stbi_image_free(input_image);
    stbi_image_free(downsampled_image);
    // Finalize MPI
    MPI_Finalize();
    return 0;
}


void paralel_downscaling(uint8_t* rgb_image, int width, int height, uint8_t* downsampled_image, int rank, int size) {
    //Calculating downsampled image size
    int downsampled_width = width / 2;
    int downsampled_height = height / 2;
    //Calculating the number of pixels per process
    int pixels_per_process = (downsampled_width * downsampled_height) / size;
    int start_index = rank * pixels_per_process;
    int end_index = start_index + pixels_per_process;
    //Calculating the start and end index for the last process
    if (rank == size - 1) {
        end_index = downsampled_width * downsampled_height;
    }

    //Downsampling the image in this for loop!!
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
    MPI_Allgather(MPI_IN_PLACE, pixels_per_process * CHANNEL_NUM, MPI_UNSIGNED_CHAR,
                  downsampled_image, pixels_per_process * CHANNEL_NUM, MPI_UNSIGNED_CHAR,
                  MPI_COMM_WORLD);
}