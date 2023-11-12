# Makefile - makefile for UDP forwarder

# setup environment
CPU_CORE       := cortex-a9
OPTIMIZE_LEVEL := 2
BUILD_CONFIG   :=
V2X_SDK_PATH   :=/home/dyonyon/Desktop/ETRI/v2x-sdk

export CPU_CORE OPTIMIZE_LEVEL BUILD_CONFIG

include $(V2X_SDK_PATH)/include/make/rules.make


PROGRAM = udpfwd
SRCS    = udpfwd.c

# Add required library
LDFLAGS +=

.PHONY: all build clean

all: build

build:
	@echo build $(PROGRAM) ...
	$(CC) $(CFLAGS) $(SRCS) -o $(PROGRAM) $(LDFLAGS)

clean:
	rm -f *.o $(PROGRAM)
