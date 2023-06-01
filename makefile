ARGS=-lmpi_cxx -lm -lstdc++ -lhwloc -fopenmp
CC = mpicc

_RARGS=papagan.JPG out.jpg
PHOTODIR=./photos
RARGS=$(patsubst %,$(PHOTODIR)/%,$(_RARGS))

_HEADERS=stb_image.h stb_image_write.h
INCLUDEDIR=./include
HEADERS=$(patsubst %,$(INCLUDEDIR)/%,$(_HEADERS))
INCLUDE=-I./include

ODIR=./bin
SRC=./src


.PHONY: all
all: $(patsubst $(SRC)/%.cpp, $(ODIR)/%, $(wildcard $(SRC)/*.cpp))

$(ODIR)/%: $(SRC)/%.cpp $(HEADERS)
	$(CC) $< -o $@ $(CFLAGS) $(INCLUDE) $(ARGS)
.PHONY: run

define run_target

runSequential$(1): $(ODIR)/seq_main
	mpirun -np $(1) $(ODIR)/seq_main $(RARGS)
.PHONY: runSequential$(1)

ifeq ($(1),0)
    ANOTHER_VALUE = 16
else ifeq ($(1),1)
    ANOTHER_VALUE = 8
else ifeq ($(1),3)
    ANOTHER_VALUE = 4
else ifeq ($(1),7)
    ANOTHER_VALUE = 2
else ifeq ($(1),15)
    ANOTHER_VALUE = 1
endif
runCuda: $(ODIR)/cuda_main
	nvcc $(SRC)/cuda_main.cu -o $(ODIR)/cuda_main $(INCLUDE) $(ARGS)
	./$(ODIR)/cuda_main $(RARGS) $(1)
.PHONY: runCuda
runOpenmp$(1): $(ODIR)/openmp_main
	$(ODIR)/openmp_main $(RARGS) $(1) 
.PHONY: runOpenmp$(1)

runHybrid$(1): $(ODIR)/hybrid_main
	mpirun -np $(1) $(ODIR)/hybrid_main $(RARGS) $(ANOTHER_VALUE)
.PHONY: runHybrid$(1)
endef

$(foreach proc,0 1 2 3 4 6 8 10 12 14 16,$(eval $(call run_target,$(proc))))

.PHONY: clean

clean:
	rm -f bin/* photos/out.jpg
	clear