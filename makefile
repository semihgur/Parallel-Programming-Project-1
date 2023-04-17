ARGS=-lmpi_cxx -lm -lstdc++ -lhwloc
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
	mpicc $< -o $@ $(INCLUDE) $(ARGS)


.PHONY: run

define run_target
runSequential$(1): all
	mpirun -np $(1) $(ODIR)/seq_main $(RARGS)

.PHONY: runSequential$(1)

runParallel$(1): all
	mpirun -np $(1) $(ODIR)/mpi_main $(RARGS)

.PHONY: runParallel$(1)
endef

$(foreach proc,1 2 4,$(eval $(call run_target,$(proc))))



.PHONY: clean
clean:
	rm -f bin/* photos/out.jpg
