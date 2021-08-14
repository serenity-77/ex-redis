CURRENT_DIR := $(abspath $(patsubst %/, %, $(dir $(abspath $(lastword $(MAKEFILE_LIST))))))

TARGET_DIR := $(CURRENT_DIR)

# VERSION := 1.1.8
TARGET_NAME := ex.so
TARGET := $(TARGET_DIR)/$(TARGET_NAME)

COMPILER = gcc
COMPILER_FLAGS = -fPIC -std=gnu99

LINKER = ld
LINKER_FLAGS = -shared -Bsymbolic -lc

all: build

build:
	$(COMPILER) $(COMPILER_FLAGS) -c -o ex.o ex.c
	$(COMPILER) $(COMPILER_FLAGS) -c -o utils.o utils.c
	$(LINKER) -o $(TARGET_NAME) ex.o utils.o $(LINKER_FLAGS)

clean:
	rm $(TARGET)
