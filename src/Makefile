include Make_linux.inc
#include Make_msys2.inc
#include Make_osx.inc

# Build through MPI wrapper when available so mpi.h symbols link cleanly.
ifneq ($(strip $(MPICXX)),)
CXX := $(MPICXX)
endif

CXXFLAGS = -std=c++17

# Debug mode is enabled only for explicit truthy values.
# This avoids accidental debug builds when DEBUG is set to unrelated values
# in the shell environment (e.g. DEBUG=release).
ifneq ($(filter 1 yes YES true TRUE,$(DEBUG)),)
CXXFLAGS += -g -O0 -Wall -fbounds-check -pedantic -D_GLIBCXX_DEBUG
CXXFLAGS2 := $(CXXFLAGS)
else
CXXFLAGS2 := $(CXXFLAGS) -O2 -march=native -Wall
CXXFLAGS += -O3 -march=native -Wall
endif

ALL= ant_simu.exe 

default:	help

all: $(ALL)

clean:
	@rm -fr *.o app/*.o sim/*.o mpi/*.o mpi2/*.o *.exe *~

.cpp.o:
	$(CXX) $(CXXFLAGS2) -c $^ -o $@	

ant_simu.exe : ant.o ants_soa.o fractal_land.o renderer.o window.o timing_profile.o app/cli.o sim/backend.o sim/aos_backend.o sim/soa_backend.o sim/omp_soa_backend.o sim/mpi1_soa_backend.o sim/mpi2_soa_backend.o sim/runner.o mpi/mpi_runtime.o mpi2/local_pheromone_grid.o ant_simu.o
	$(CXX) $(CXXFLAGS2) $^ -o $@ $(LIB)	

help:
	@echo "Available targets : "
	@echo "    all            : compile all executables"
	@echo "Add DEBUG=yes (or DEBUG=1) to compile in debug"
	@echo "Configuration :"
	@echo "    CXX      :    $(CXX)"
	@echo "    CXXFLAGS :    $(CXXFLAGS)"

%.html: %.md
	pandoc -s --toc $< --css=./github-pandoc.css --metadata pagetitle="OS202 - TD1" -o $@
