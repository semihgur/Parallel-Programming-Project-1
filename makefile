CC = gcc
NVCC = nvcc
MPICC = mpicc

CFLAGS = -I./include -lm -lstdc++ -lmpi_cxx
CXXFLAGS = -std=c++11 $(CFLAGS) 
NVFLAGS = -lmpi_cxx -lm -lstdc++ -lhwloc -I./include

all: openmp cuda seq 

openmp: src/openmp_main.cpp
	$(CC) src/openmp_main.cpp -o bin/openmp_main $(CFLAGS) -fopenmp

cuda: src/cuda.cu 
	$(NVCC) -o ./bin/cuda src/cuda.cu $(NVFLAGS)

seq: src/seq_main.cpp 
	$(MPICC) -o ./bin/seq_main src/seq_main.cpp $(CFLAGS)

.PHONY: clean

clean:  
	rm -rf bin/* ./photos/out.jpg

run_openmp:  
	./bin/openmp_main ./photos/papagan.JPG ./photos/out.jpg 16 

run_cuda: 
	./bin/cuda ./photos/papagan.JPG ./photos/out.jpg

run_seq:  
	mpirun -n 1 ./bin/seq_main ./photos/papagan.JPG ./photos/out.jpg
