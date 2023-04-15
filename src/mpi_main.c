#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

int main(int argc, char* argv[]) {
    unsigned char *input_image = NULL;

    // Init MPI library
    MPI_Init(&argc, &argv);
    int width, height, channels;

    // Determine rank and size (process count)
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // For process 0 read image
    if (rank == 0) {


        input_image = stbi_load(argv[1], &width, &height, &channels, 0);
        printf("Width: %d  Height: %d \n",width,height);
	    printf("Input: %s , Output: %s  \n",argv[1],argv[2]);
        if (input_image == NULL) {
            printf("Error: Failed to load image '%s'\n", argv[1]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    // Broadcast Image information to other process
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&channels, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Divide the input image into equally-sized chunks based on the number of processes,
    // and scatter them to all processes
    int chunk_size = (height / size + (height % size != 0 ? 1 : 0)) * width * channels;
    unsigned char *input_chunk = malloc(chunk_size);

    MPI_Scatter(input_image, chunk_size, MPI_UNSIGNED_CHAR, input_chunk, chunk_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        stbi_image_free(input_image);
    }

    double time1 = MPI_Wtime();

    // Downscale the image chunk on each process
    int output_width = width / 2;
    int output_height = height / 2;
    int output_chunk_size = (output_height / size + (output_height % size != 0 ? 1 : 0)) * output_width * channels;
    unsigned char *output_chunk = malloc(output_chunk_size);

    // Downscaling chunks
    for (int y = 0; y < output_height / size; y++) {
        for (int x = 0; x < output_width; x++) {
            int input_x = x * 2;
            int input_y = y * 2 + rank * (height / size);
            int pixel_index = (input_y * width + input_x) * channels;

            // Calculate the average color of the 2x2 block
            int r = (input_chunk[pixel_index] + input_chunk[pixel_index + channels] + input_chunk[pixel_index + width * channels] + input_chunk[pixel_index + (width + 1) * channels]) / 4;
            int g = (input_chunk[pixel_index + 1] + input_chunk[pixel_index + channels + 1] + input_chunk[pixel_index + width * channels + 1] + input_chunk[pixel_index + (width + 1) * channels + 1]) / 4;
            int b = (input_chunk[pixel_index + 2] + input_chunk[pixel_index + channels + 2] + input_chunk[pixel_index + width * channels + 2] + input_chunk[pixel_index + (width + 1) * channels + 2]) / 4;

            // Assign the average color to the output pixel
            int output_index = (y * output_width * channels) + (x * channels);
            output_chunk[output_index] = r;
            output_chunk[output_index + 1] = g;
            output_chunk[output_index + 2] = b;
        }
    }

    double time2 = MPI_Wtime();

    // Gather the output chunks from all processes on the root process
    unsigned char *output_image = NULL;

    if (rank == 0) {
        output_image = malloc(output_width * output_height * channels);
    }

    MPI_Gather(output_chunk, output_chunk_size, MPI_UNSIGNED_CHAR, output_image, output_chunk_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // Save the output image on the root process
    if (rank == 0) {
        stbi_write_jpg(argv[2], output_width, output_height, channels, output_image, output_width * channels);
        printf("Width: %d  Height: %d \n", width, height);
        printf("Input: %s , Output: %s  \n", argv[1], argv[2]);
        printf("Elapsed time: %lf \n", time2 - time1);
    }

    // Free the memory used by the input and output image data and finalize MPI
    free(input_chunk);
    free(output_chunk);

    if (rank == 0) {
        free(output_image);
    }

    MPI_Finalize();

    return 0;
}
