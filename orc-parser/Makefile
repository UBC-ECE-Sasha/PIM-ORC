CC = g++
CFLAGS = -std=c++17 -O3 -g -pthread -Wall 
DPU_OPTS = `dpu-pkg-config --cflags --libs dpu` 
SOURCE = reader.cc

.PHONY: default all clean reader
default: reader

all: reader

clean:
	rm ./reader
	rm -rf build/

NR_TASKLETS = 1
USE_PIM = 0
BLOCK_SIZE = 32 * 1024

SNAPPY_HOME := $(shell pwd)/build/snappy
SNAPPY_SRC  := $(shell cd ../snappy; pwd)
DPU_PROGRAM := \"$(SNAPPY_SRC)/pim-snappy/decompress.dpu\"

SNAPPY_LIB := ../build/snappy/libsnappy.a
$(SNAPPY_LIB):
	cd $(SNAPPY_SRC)/pim-snappy && make NR_TASKLETS=$(NR_TASKLETS)
	mkdir -p build
	cd build && mkdir -p snappy
	cd build/snappy && cmake -DBLOCK_SIZE=$(BLOCK_SIZE) -DUSE_PIM=$(USE_PIM) -DNR_TASKLETS=$(NR_TASKLETS) -DDPU_PROGRAM=$(DPU_PROGRAM) -DCMAKE_INSTALL_PREFIX=$(SNAPPY_HOME) -DCMAKE_INSTALL_LIBDIR=$(SNAPPY_HOME) ../../../snappy && make 

APACHE_ORC_LIB := build/c++/src/liborc.a
$(APACHE_ORC_LIB):
	cp ../orc/c++/include/orc/orc-config.hh.in ../orc/c++/include/orc/orc-config.hh
	sed -i 's/cmakedefine/define/g' ../orc/c++/include/orc/orc-config.hh
	mkdir -p build
	cd build && cmake -DBUILD_JAVA=OFF -DSNAPPY_HOME=$(SNAPPY_HOME) -DSNAPPY_INCLUDE_DIR=$(SNAPPY_SRC) ../../orc && make -j4 package

INCLUDES := -I../snappy -I../orc/c++/include -I../orc/c++/src -I../orc/c++/src/sargs -I../orc/c++/src/wrap -I $(shell pwd)

LIBS = $(shell find . -name *.a)

reader: $(SOURCE) $(SNAPPY_LIB) $(APACHE_ORC_LIB)
	$(CC) $(CFLAGS) -o $@ $(SOURCE) $(LIBS) -DBLOCK_SIZE=$(BLOCK_SIZE) -DUSE_PIM=$(USE_PIM) $(INCLUDES) -lsnappy -L./build/snappy $(DPU_OPTS)
