# Makefile for simple-test example using cpm
# This example demonstrates using cpm to manage dependencies

CC = cc
CFLAGS = -Wall -Wextra -O2
LDFLAGS =

# Include the cpm-generated Makefile fragment (after defining CFLAGS to use +=)
-include .cpm/cpm.mk

TARGET = simple-test

# Source files
SRCS = main.c
OBJS = $(SRCS:.c=.o)

# Default target: build with cpm support
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Install dependencies using cpm
deps:
	cpm get github.com/callowaysutton/sht-lib@main

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Clean cpm cache and vendor directory
distclean: clean
	cpm clean

.PHONY: all deps clean distclean