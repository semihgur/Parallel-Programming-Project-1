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

long get_current_core_frequency();
void downscaling(uint8_t* rgb_image,int width, int height,uint8_t* downsampled_image);

int main(int argc, char* argv[]){

    // Inıt MPI 
    MPI_Init(&argc,&argv);

    // Determine rank and size (process count)
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    // Image Informations
    int width, height, channels;
    unsigned char *input_image ;
    int output_width;
    int output_height;

    unsigned char *input_chunk;



    if(rank == 0){  // Master process
        // Read input image

        input_image = stbi_load(argv[1],&width,&height,&channels,0);

        output_width = width / 2;
        output_height = height / 2;

        printf("Width: %d  Height: %d \n",width,height);
      

        // Receive frequency information from worker processes
        long *frequencies = (long*)malloc((size - 1) * sizeof(long));
        for (int i =1 ; i< size ; i++){
            // Recive all of them from other proccessor
            MPI_Recv(&frequencies[i - 1 ], 1, MPI_LONG,i , 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            //printf("%ld\n", frequencies[i - 1 ]);

        }

        
        // Master process de imagge proseesing yapcak
        

        // Calculate total frequency
        long total_frequency = 0;
        for (int i = 0; i < size - 1; i++) {
            total_frequency += frequencies[i];
        }
        

        // Calculate workload distribution
        int *workload_sizes = (int*)malloc((size - 1) * sizeof(int));
        for (int i = 0; i < size - 1; i++) {
            workload_sizes[i] = (height / size) * (float)(frequencies[i] * ( 1.0 / total_frequency));
        }

        
        // Send image portions to worker processes
        int offset = 0;
        for (int i = 1; i < size; i++) {
            int chunk_size = workload_sizes[i - 1] * width * channels;
            MPI_Send(&chunk_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&input_image[offset], chunk_size, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
            offset += chunk_size;
        }

        // Send width of immage to other processor
        for (int i = 1; i < size; i++) {
            MPI_Send(&width, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        /*
        // Receive downscaled image portions and combine them into a single image
        unsigned char *output_image = (unsigned char *)malloc(output_width * output_height * channels);
        int output_offset = 0;
        for (int i = 1; i < size; i++) {
            int output_chunk_size = output_width * workload_sizes[i - 1] * channels;
            MPI_Recv(&output_image[output_offset], output_chunk_size, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            output_offset += output_chunk_size;
        }

        // Save the output image
        stbi_write_jpg(argv[3], output_width, output_height, channels, output_image, output_width * channels);
        free(output_image);

        */
        // Clean up
        free(input_image);
        free(frequencies);
        //free(workload_sizes);
        


    }else{


        // Get performance info and sent it to Master process
        long frequencyOfCurrent = get_current_core_frequency();


        // Send frequency to Master process to recive partition to handle.
        MPI_Send(&frequencyOfCurrent, 1 , MPI_LONG, 0,0,MPI_COMM_WORLD);
        
        
        
        // Receive the assigned image portion and its size
        int assigned_chunk_size;
        MPI_Recv(&assigned_chunk_size, 1 , MPI_INT, 0 , 0 ,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        unsigned char *input_chunk = (unsigned char *)malloc(assigned_chunk_size );
        MPI_Recv(input_chunk, assigned_chunk_size, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);


        // recieve width from master proccess
        int width;
        MPI_Recv(&width, 1 , MPI_INT, 0 , 0 ,MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Donwscale image partition
        // Downscale the assigned image portion
        int output_chunk_size = assigned_chunk_size / 4;
        unsigned char *output_chunk = (unsigned char *)malloc(output_chunk_size);
        
        /* Segment foult here */
        downscaling(input_chunk, width ,assigned_chunk_size,output_chunk);
        
        // Send the downscaled image portion back to the master process
        MPI_Send(output_chunk, output_chunk_size, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);

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


/*
    This function return current processor frequency.
    So we can determine the performance of the proccesor.
    Because its frequency is directly proportional to its performance.s
*/
long get_current_core_frequency() {
    FILE *cpuinfo = fopen("/proc/cpuinfo", "rb");
    if (cpuinfo == NULL) {
        perror("Failed to open /proc/cpuinfo");
        return -1;
    }

    long frequency = -1;
    char line[256];
    while (fgets(line, sizeof(line), cpuinfo)) {
        if (sscanf(line, "cpu MHz : %ld", &frequency) == 1) {
            break;
        }
    }

    fclose(cpuinfo);

    return frequency;
}




// Got segmentation fould should be fix
void downscaling(uint8_t* rgb_image,int width, int chunkSize,uint8_t* downsampled_image)
{	
    int downsampled_width = width / 2;
    int downsampled_height = chunkSize / (width  / 4 /* Channel */) ;

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