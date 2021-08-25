CURRENT_DIR := $(abspath $(patsubst %/, %, $(dir $(abspath $(lastword $(MAKEFILE_LIST))))))

TARGET_DIR := $(CURRENT_DIR)

# VERSION := 1.1.8
TARGET_NAME := ex.so
TARGET := $(TARGET_DIR)/$(TARGET_NAME)

COMPILER = gcc
COMPILER_FLAGS = -fPIC -std=gnu99

LINKER = ld
LINKER_FLAGS = -shared -Bsymbolic -lc -lcrypto

all: build

build:
	$(COMPILER) $(COMPILER_FLAGS) -c -o bnfp.o bnfp.c
	$(COMPILER) $(COMPILER_FLAGS) -c -o ex.o ex.c -DUSE_BNFP
	# $(COMPILER) $(COMPILER_FLAGS) -c -o utils.o utils.c
	$(LINKER) -o $(TARGET_NAME) ex.o bnfp.o $(LINKER_FLAGS)

clean:
	rm -rf $(TARGET)
	rm -rf utils.o
	rm -rf ex.o

test_bnfp:
	mkdir -p test_obj
	$(COMPILER) -g bnfp.c -o test_obj/bnfp -lcrypto -DTEST_MAIN
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes test_obj/bnfp
	rm -rf test_obj
