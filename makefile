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

.PHONY: run

run:
	mpirun -np 1 $(ODIR)/program $(RARGS)

.PHONY: clean

clean:
	rm -f bin/* photos/out.jpg