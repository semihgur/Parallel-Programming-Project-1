/**
 *
 * CENG342 Project-1
 *
 * Downscaling 
 *
 * Usage:  main <input.jpg> <output.jpg> 
 *
 * @group_id 00
 * @author  your names
 *
 * @version 1.0, 02 April 2022
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

void seq_downscaling(uint8_t* rgb_image,int width, int height,uint8_t* downsampled_image);
//Do not use global variables

int main(int argc,char* argv[]) 
{	
	MPI_Init(&argc,&argv);
    int width, height, bpp;
	
	// Reading the image in grey colors
    uint8_t* input_image = stbi_load(argv[1], &width, &height, &bpp, CHANNEL_NUM);
	
    printf("Width: %d  Height: %d \n",width,height);
	printf("Input: %s , Output: %s  \n",argv[1],argv[2]);
	
	// start the timer
	double time1= MPI_Wtime();	
	uint8_t* downsampled_image=(uint8_t*) malloc(width/2*height/2*CHANNEL_NUM*sizeof(uint8_t));
	seq_downscaling(input_image,width, height,downsampled_image);
	    
	double time2= MPI_Wtime();
	printf("Elapsed time: %lf \n",time2-time1);	
	
	// Storing the image 
    stbi_write_jpg(argv[2], width/2, height/2, CHANNEL_NUM, downsampled_image, 100);
    stbi_image_free(input_image);
	stbi_image_free(downsampled_image);

	MPI_Finalize();
    return 0;
}


void seq_downscaling(uint8_t* rgb_image,int width, int height,uint8_t* downsampled_image)
{	
int downsampled_width = width / 2;
    int downsampled_height = height / 2;

    // iterate through the image pixels in blocks of size 2x2 and calculate the average
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