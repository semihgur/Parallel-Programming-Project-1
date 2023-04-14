ARGS=-lmpi_cxx -lm -lstdc++
_RARGS=papagan.JPG out.jpg
PHOTODIR=./photos
RARGS=$(patsubst %,$(PHOTODIR)/%,$(_RARGS))
_HEADERS=stb_image.h stb_image_write.h
INCLUDEDIR=./include
HEADERS=$(patsubst %,$(INCLUDEDIR)/%,$(_HEADERS))
INCLUDE=-I./include
ODIR=./bin

program: main.cpp $(HEADERS)
	mpicc $< -o $(ODIR)/$@ $(INCLUDE) $(ARGS)

programPar: mpi_main.cpp $(HEADERS)
	mpicc $< -o $(ODIR)/$@ $(INCLUDE) $(ARGS)
.PHONY: run

runParallel:
	mpirun -np 1 $(ODIR)/programPar $(RARGS)
.PHONY: clean

runParallel2:
	mpirun -np 2 $(ODIR)/programPar $(RARGS)
.PHONY: clean

runParallel4:
	mpirun -np 4 $(ODIR)/programPar $(RARGS)
.PHONY: clean

runSequential:
	mpirun -np 1 $(ODIR)/program $(RARGS)

.PHONY: clean

runSequential2:
	mpirun -np 2 $(ODIR)/program $(RARGS)

.PHONY: clean

runSequential4:
	mpirun -np 4 $(ODIR)/program $(RARGS)
.PHONY: clean

clean:
	rm -f bin/* photos/out.jpg