ARGS=-lmpi_cxx -lm -lstdc++ -lhwloc -fopenmp
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
	gcc $(SRC)/openmp_main.cpp -o $(ODIR)/openmp_main $(INCLUDE) $(ARGS)
	mpicc $(SRC)/seq_main.cpp -o $(ODIR)/seq_main $(INCLUDE) $(ARGS)
.PHONY: run

define run_target

runSequential$(1): $(ODIR)/seq_main
	mpirun -np $(1) $(ODIR)/seq_main $(RARGS)
.PHONY: runSequential$(1)

runOpenmp$(1): $(ODIR)/openmp_main
	$(ODIR)/openmp_main $(RARGS) $(1)
.PHONY: runOpenmp$(1)

runHybrid$(1): $(ODIR)/hybrid_main
	mpirun -np $(1) $(ODIR)/hybrid_main $(RARGS)
.PHONY: runHybrid$(1)

endef

$(foreach proc,1 2 3 4 6 8 10 12 14 16,$(eval $(call run_target,$(proc))))

.PHONY: clean

clean:
	rm -f bin/* photos/out.jpg
	clear