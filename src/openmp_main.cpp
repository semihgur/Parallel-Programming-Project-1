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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#define CHANNEL_NUM 4

void parallel_downscaling(uint8_t* rgb_image, int width, int height, uint8_t* downsampled_image, int num_threads);

int main(int argc, char* argv[]) 
{

    int width, height, bpp;

    // Reading the image in grey colors
    uint8_t* input_image = stbi_load(argv[1], &width, &height, &bpp, CHANNEL_NUM);

    printf("Width: %d  Height: %d \n", width, height);
    printf("Input: %s , Output: %s  \n", argv[1], argv[2]);

    int num_threads = atoi(argv[3]);
    // start the timer
    double time1 = omp_get_wtime();	
    uint8_t* downsampled_image = (uint8_t*) malloc(width/2 * height/2 * CHANNEL_NUM * sizeof(uint8_t));
    parallel_downscaling(input_image, width, height, downsampled_image, num_threads);

    double time2 = omp_get_wtime();
    printf("Elapsed time: %lf \n", time2 - time1);	

    // Storing the image 
    stbi_write_jpg(argv[2], width/2, height/2, CHANNEL_NUM, downsampled_image, 100);
    stbi_image_free(input_image);
    stbi_image_free(downsampled_image);

    return 0;
}

void parallel_downscaling(uint8_t* rgb_image, int width, int height, uint8_t* downsampled_image, int thread_count)
{	
    int downsampled_width = width / 2;
    int downsampled_height = height / 2;

    // iterate through the image pixels in blocks of size 2x2 and calculate the average
    #pragma omp parallel for collapse(2) schedule(guided,1000) num_threads(thread_count)
    for (int y = 0; y < downsampled_height; y++) {
        for (int x = 0; x < downsampled_width; x++) {
            int r = 0, g = 0, b = 0, a = 0;

            // calculate the average value of each channel in the 2x2 block
            for (int dy = 0; dy < 2; dy++) {
                for (int dx = 0; dx < 2; dx++) {
                    int pixel_index = ((y * 2 + dy) * width + (x * 2 + dx)) * 4;

                    r += rgb_image[pixel_index];
                    g += rgb_image[pixel_index + 1];
                    b += rgb_image[pixel_index + 2];
                    a += rgb_image[pixel_index + 3];
                }
            }

            // calculate the average value for each channel and store it in the downsampled image
            int downsampled_pixel_index = (y * downsampled_width + x) * 4;
            downsampled_image[downsampled_pixel_index] = r / 4;
            downsampled_image[downsampled_pixel_index + 1] = g / 4;
            downsampled_image[downsampled_pixel_index + 2] = b / 4;
            downsampled_image[downsampled_pixel_index + 3] = a / 4;
        }
    }
}
