#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <hwloc.h>          // delete
#include <sched.h>




#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#define PERFORMANCE_CORE_RATIO 1
#define EFFICIENCY_CORE_RATIO 0

#include "stb_image.h"
#include "stb_image_write.h"

typedef struct {
    int start_row;
    int end_row;
} Task;


int get_current_core_frequency();
void downscale_image_portion(const unsigned char *input_chunk, int input_width, int input_height, int channels,
                             unsigned char *output_chunk, int output_width, int output_height);

int main(int argc, char* argv[]){

    // Inıt MPI 
    MPI_Init(&argc,&argv);

    // Determine rank and size (process count)
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    // Image Informations
    int width, height, channels;
    unsigned char *input_image = NULL;
    int output_width = width / 2;
    int output_height = height / 2;


    if(rank == 0){  // Master process
        // Read input image
        input_image = stbi_load(argv[0],&width,&height,&channels,0);

        output_width = width / 2;
        output_height = height / 2;


        // Receive frequency information from worker processes
        int *frequencies = malloc((size - 1) * sizeof(unsigned long));
        for (int i =1 ; i< size ; i++){
            // Recive all of them from other proccessor
            MPI_Recv(&frequencies[i - 1 ], 1, MPI_INT,i , 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }


        // Send the input image size to the worker processes
        for (int i = 1; i < size; i++) {
            MPI_Send(&width, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&height, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
        

        // Calculate total frequency
        int total_frequency = 0;
        for (int i = 0; i < size - 1; i++) {
            total_frequency += frequencies[i];
        }

        // Calculate workload distribution
        int *workload_sizes = malloc((size - 1) * sizeof(int));
        for (int i = 0; i < size - 1; i++) {
            workload_sizes[i] = (height / size) * (frequencies[i] / (float)total_frequency);
        }


        // Send image portions to worker processes
        int offset = 0;
        for (int i = 1; i < size; i++) {
            int chunk_size = workload_sizes[i - 1] * width * channels;
            MPI_Send(&input_image[offset], chunk_size, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
            offset += chunk_size;
        }

        // Receive downscaled image portions and combine them into a single image
        unsigned char *output_image = malloc(output_width * output_height * channels);
        int output_offset = 0;
        for (int i = 1; i < size; i++) {
            int output_chunk_size = output_width * workload_sizes[i - 1] * channels;
            MPI_Recv(&output_image[output_offset], output_chunk_size, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            output_offset += output_chunk_size;
        }

        // Save the output image
        stbi_write_jpg(argv[2], output_width, output_height, channels, output_image, output_width * channels);
        free(output_image);

        // Clean up
        free(input_image);
        free(frequencies);
        free(workload_sizes);


    }else{
        output_width = width / 2;
        output_height = height / 2;

        // Get performance info and sent it to Master process
        int frequencyOfCurrent = get_current_core_frequency();

        // Send frequency to Master process to recive partition to handle.
        MPI_Send(&frequencyOfCurrent, 1 , MPI_INT, 0,0,MPI_COMM_WORLD);

         // Receive the input image size
        MPI_Recv(&width, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&height, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        output_width = width / 2;
        output_height = height / 2;


        // Receive the assigned image portion and its size
        int assigned_workload, assigned_chunk_size;
        MPI_Recv(&assigned_workload, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        assigned_chunk_size = assigned_workload * width * channels;
        unsigned char *input_chunk = malloc(assigned_chunk_size);
        MPI_Recv(input_chunk, assigned_chunk_size, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    
        // Donwscale image partition

        // Downscale the assigned image portion
        int output_chunk_size = output_width * assigned_workload * channels;
        unsigned char *output_chunk = malloc(output_chunk_size);
        downscale_image_portion(input_chunk, width, assigned_workload, channels, output_chunk, output_width, output_height / size);

        // Send the downscaled image portion back to the master process
        MPI_Send(output_chunk, output_chunk_size, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);

        // Clean up
        free(input_chunk);
        free(output_chunk);

    }


    // Save and Clean up for the master process
    if (rank == 0) {
        stbi_write_jpg(argv[2], width/2, height/2, channels, input_image, 100);
        stbi_image_free(input_image);

        free(input_image);
    }


    // Finalize MPI
    MPI_Finalize();

    return 0;

}






/*
    This function return current processor frequency.
    So we can determine the performance of the proccesor.
    Because its frequency is directly proportional to its performance.s
*/
int get_current_core_frequency() {
    hwloc_topology_t topology;
    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);
    
    hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
    hwloc_get_cpubind(topology, cpuset, HWLOC_CPUBIND_THREAD);

    int current_core = -1;
    hwloc_obj_t obj = hwloc_get_obj_inside_cpuset_by_depth(topology, cpuset, 0, 0);
    if (obj && obj->type == HWLOC_OBJ_PU) {
        current_core = obj->os_index;
    }

    hwloc_bitmap_free(cpuset);
    hwloc_topology_destroy(topology);

    return current_core;
}




void downscale_image_portion(const unsigned char *input_chunk, int input_width, int input_height, int channels,
                             unsigned char *output_chunk, int output_width, int output_height) {
    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            int input_x = x * 2;
            int input_y = y * 2;
            int pixel_index = (input_y * input_width + input_x) * channels;

            // Calculate the average color of the 2x2 block
            int r = (input_chunk[pixel_index] + input_chunk[pixel_index + channels] + input_chunk[pixel_index + input_width * channels] + input_chunk[pixel_index + (input_width + 1) * channels]) / 4;
            int g = (input_chunk[pixel_index + 1] + input_chunk[pixel_index + channels + 1] + input_chunk[pixel_index + input_width * channels + 1] + input_chunk[pixel_index + (input_width + 1) * channels + 1]) / 4;
            int b = (input_chunk[pixel_index + 2] + input_chunk[pixel_index + channels + 2] + input_chunk[pixel_index + input_width * channels + 2] + input_chunk[pixel_index + (input_width + 1) * channels + 2]) / 4;

            // Assign the average color to the output pixel
            int output_index = (y * output_width + x) * channels;
            output_chunk[output_index] = r;
            output_chunk[output_index + 1] = g;
            output_chunk[output_index + 2] = b;
        }
    }
}

