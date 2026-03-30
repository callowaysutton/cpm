# Makefile for app example using cpm

CC = cc
CFLAGS = -Wall -Wextra -O2
LDFLAGS =

-include .cpm/cpm.mk

TARGET = app

SRCS = main.c $(wildcard .cpm/vendor/*/src/*.c) $(wildcard .cpm/vendor/.*@*/src/*.c) $(wildcard .cpm/vendor/*/*.c) $(wildcard .cpm/vendor/.*@*/*.c)
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

deps:
	../../cpm get ../lib-a@main

clean:
	rm -f $(OBJS) $(TARGET)

distclean: clean
	rm -rf .cpm

.PHONY: all deps clean distclean
