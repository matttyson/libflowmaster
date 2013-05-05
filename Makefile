
CC=gcc
CFLAGS= -g -Wall -Wno-unused-variable -DFM_BUILDING_DLL  -fPIC -fvisibility=hidden
# -flto -fuse-linker-plugin

#CFLAGS+=-DFM_DEBUG_LOGGING

OBJECTS=\
	flowmaster.o\
	flowmaster_linux.o\
	flash.o

LIBFLOW=libflowmaster.so

.SUFFIXES: .o .c
.PHONY: clean

all: $(LIBFLOW)

$(LIBFLOW): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(LIBFLOW) -shared -Wl,-soname,$(LIBFLOW) $(OBJECTS)

testflash: $(LIBFLOW) testflash.o
	$(CC) -Wall -g -o $@ testflash.o -L. -lflowmaster

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(LIBFLOW) test.o test testflash testflash.o
