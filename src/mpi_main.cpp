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

void downscaling(uint8_t *rgb_image, int width, u_int64_t height, uint8_t *downsampled_image);

int main(int argc, char *argv[])
{

    // Inıt MPI Bismillahirrahmaniraahhimm
    MPI_Init(&argc, &argv);

    // Determine rank and size (process count)
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    if (rank == 0)
    { // Master process
        // Image Informations
        int width, height, channels;
        unsigned char *input_image;
        int output_width;
        int output_height;

        // Read input image
        
        input_image = stbi_load(argv[1], &width, &height, &channels, 0);    
        channels=4;
        output_width = width / 2;
        output_height = height / 2;

        printf("Width: %d  Height: %d \n", width, height);

        // Calculate workload for each rank
        int* divided_line_count = (int *)malloc((size) * sizeof(int));
        for(int i=0;i<size;i++){
            divided_line_count[i] = height / size;
        }
        divided_line_count[size-1] += height % size;
        //prining divided line count
        for(int i=0;i<size;i++){
            printf("Divided line count: %d \n", divided_line_count[i]);
        }



        // Send image portions to worker processes
        int offset = divided_line_count[0] * width * channels;
        for (int i = 1; i < size; i++)
        {
            int eight_bit_int_count = divided_line_count[i] * width * channels;
            MPI_Send(&eight_bit_int_count, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            printf("input_image[offset]: %d \n", offset);
            MPI_Send(&input_image[offset], eight_bit_int_count, MPI_UINT8_T, i, 0, MPI_COMM_WORLD);
            MPI_Send(&width, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            offset += eight_bit_int_count;
        }
        u_int8_t *output_image = (u_int8_t *)malloc(output_width * output_height * channels * sizeof(u_int8_t));
        printf("Chunk sizaaaaaaaaaaaaaaaaaaaaaaaaaaaaae: %d \n", divided_line_count[0] * width * channels);

        downscaling(input_image, width,(u_int64_t) divided_line_count[0] * width * channels, output_image);



        
        // Receive downscaled image portions and combine them into a single image
        int output_offset = output_width * divided_line_count[0] * channels;
        int output_eight_bit_int_count;
        for (int i = 1; i < size; i++) {
            output_eight_bit_int_count = divided_line_count[i] * output_width * channels;
            MPI_Recv(&output_image[output_offset], output_eight_bit_int_count, MPI_UINT8_T, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            output_offset += output_eight_bit_int_count;
        }

        // Save the output image
        stbi_write_jpg(argv[2], output_width, output_height, channels, output_image, output_width * channels);
        free(output_image);

        
        // Clean up
        free(input_image);

        // free(workload_Ratios);
    }
    else
    {

        // Receive the assigned image portion and its size
        int assigned_eight_bit_int_count;
        MPI_Recv(&assigned_eight_bit_int_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Assigned eight bit int count: %d \n", assigned_eight_bit_int_count);
        uint8_t*input_chunk = (uint8_t*)malloc(assigned_eight_bit_int_count*sizeof(uint8_t));
        printf("Input ch11111111111111k: %d \n", input_chunk[0]);
        MPI_Recv(input_chunk, assigned_eight_bit_int_count, MPI_UINT8_T, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Input chu22222222222nk: %d \n", input_chunk[0]);
        // recieve width from master proccess
        int width;
        MPI_Recv(&width, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Width:aaaaaaaaaaaa %d \n", width);  

        // Donwscale image partition
        // Downscale the assigned image portion
        unsigned char *output_chunk = (unsigned char *)malloc(assigned_eight_bit_int_count / 4);

        /* Segment foult here */
        printf("Chunk size: %d \n", assigned_eight_bit_int_count);
        downscaling(input_chunk, width, assigned_eight_bit_int_count, output_chunk);

        // Send the downscaled image portion back to the master process
        MPI_Send(output_chunk, assigned_eight_bit_int_count, MPI_UINT8_T, 0, 0, MPI_COMM_WORLD);

        // Clean up
        free(input_chunk);
        free(output_chunk);
    }

    /*
    // Save and Clean up for the master process
    if (rank == 0) {
        stbi_write_jpg(argv[2], width/2, height/2, channels, input_image, 100);
        stbi_image_free(input_image);

        free(input_image);
    }*/

    // Finalize MPI
    MPI_Finalize();

    return 0;
}

// Got segmentation fould should be fix
void downscaling(uint8_t *rgb_image, int width, u_int64_t chunkSize, uint8_t *downsampled_image)
{
    int downsampled_width = width / 2;
    printf("witdth: %d\nChunksize: %ld\n", width, chunkSize);
    //divided_line_count[0] * width * channels
    int downsampled_height = (chunkSize / width)/8;
    printf("downsampled_width: %d\n", downsampled_width);
    printf("downsampled_height: %d\n", downsampled_height);
    int a=0;

    // iterate through the image pixels in blocks of size 2x2 and calculate the average
    for (int y = 0; y < downsampled_height; y++)
    {
        for (int x = 0; x < downsampled_width; x++)
        {
            int r = 0, g = 0, b = 0, a = 0;

            // calculate the average value of each channel in the 2x2 block
            for (int dy = 0; dy < 2; dy++)
            {
                for (int dx = 0; dx < 2; dx++)
                {
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
        a++;
        printf("Downscaling done%d\n",a);

    }
    printf("Downscaling done\n");
}