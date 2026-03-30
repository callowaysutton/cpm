# Makefile for lib-a example using cpm

CC = cc
CFLAGS = -Wall -Wextra -O2 -Iinclude
LDFLAGS =

-include .cpm/cpm.mk

TARGET = liba.a

SRCS = src/liba.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p lib
	ar rcs lib/$@ $^
	@echo "Built static library: lib/$(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

deps:
	../../cpm get github.com/callowaysutton/sht-lib@main

clean:
	rm -f $(OBJS) lib/$(TARGET)
	rm -rf lib

distclean: clean
	rm -rf .cpm

.PHONY: all deps clean distclean
