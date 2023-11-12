# Makefile - makefile for WSM Tx/Rx example

# setup environment
CPU_CORE       := cortex-a9
OPTIMIZE_LEVEL := 2
BUILD_CONFIG   :=
V2X_SDK_PATH   :=/home/dyonyon/Desktop/ETRI/v2x-sdk


export CPU_CORE OPTIMIZE_LEVEL BUILD_CONFIG

include $(V2X_SDK_PATH)/include/make/rules.make


PROGRAM = mesh
SRCS =                    \
    wsm-input-params.c    \
    wsm-rx.c              \
    wsm-tx.c              \
    wsm-mesh.c			  

# mesh network specific source files
SRCS +=                   \
    mesh_extern.c

# Add required library
LDFLAGS += -ldot3 -lwlanaccess -lLLC -lffasn1c -laodv -lpthread -lrt
CFLAGS  += -I../../include -L../../lib/ -L.


# mesh lib source files
LIB_SRCS = mesh_extern.c aodv.c 
LIB_NAMES = libaodv.a

.PHONY: all build clean

all: lib-build build

lib-build:
	@echo build $(LIB_NAMES) ...
	$(CC) -c $(LIB_SRCS)
	ar rc $(LIB_NAMES) aodv.o mesh_extern.o

build:
	@echo build $(PROGRAM) ...
	$(CC) $(CFLAGS) $(SRCS) -o $(PROGRAM) $(LDFLAGS)

clean:
	rm -f *.o $(PROGRAM)
