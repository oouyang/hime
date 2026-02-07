# Makefile for HIME Core Library Tests
# Copyright (C) 2020 The HIME team, Taiwan

CC = gcc
CFLAGS = -Wall -Wextra -g -O0 -Wno-unused-variable
CFLAGS += -I../include -I../../tests

# The test includes the implementation via a wrapper
TEST_SRC = test-hime-core.c

# Output
TEST_BIN = test-hime-core

# Default target
all: $(TEST_BIN)

# Build test binary - compile test which includes the implementation
$(TEST_BIN): $(TEST_SRC) ../src/hime-core-impl.c ../include/hime-core.h
	$(CC) $(CFLAGS) -DHIME_VERSION=\"test\" -include ../src/hime-core-impl.c -o $@ $(TEST_SRC)

# Run tests
test: $(TEST_BIN)
	./$(TEST_BIN)

# Clean
clean:
	rm -f $(TEST_BIN)

.PHONY: all test clean
