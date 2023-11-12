# Makefile - makefile for WSM Tx/Rx example

#setup environment
CPU_CORE       := cortex-a9
OPTIMIZE_LEVEL := 2
BUILD_CONFIG   :=

export CPU_CORE OPTIMIZE_LEVEL BUILD_CONFIG

include $(V2X_SDK_PATH)/include/make/rules.make

PROGRAM = addr

SRCS =                    \
    addr.c

LDFLAGS += -ldot3 -lwlanaccess -lLLC -lffasn1c -lpthread -lrt


.PHONY: all build clean

all: build

build:
	@echo build $(PROGRAM) ...
	$(CC) $(CFLAGS) $(SRCS) -o $(PROGRAM) $(LDFLAGS)

clean:
	rm -f *.o $(PROGRAM)
