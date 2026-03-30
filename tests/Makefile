TARGET = test_runner
CC = cc
CFLAGS = -Wall -Wextra -I../include -g -fprofile-arcs -ftest-coverage -Wno-unused-function
LDFLAGS = -fprofile-arcs -ftest-coverage

-include .cpm/cpm.mk

SRC_FILES = $(filter-out ../src/main.c, $(wildcard ../src/*.c))
TEST_SRCS = main_test.c

OBJS = $(SRC_FILES:.c=.o) $(TEST_SRCS:.c=.o)

all: $(TARGET)

.cpm/cpm.mk:
	cd .. && $(MAKE) cpm
	../cpm vendor

$(TARGET): $(OBJS) .cpm/cpm.mk
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(CPM_LDFLAGS)

%.o: %.c .cpm/cpm.mk
	$(CC) $(CFLAGS) $(CPM_CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(TARGET) *.gcda *.gcno *.gcov ../src/*.gcda ../src/*.gcno .cpm cmod.lock coverage.info coverage/ *.log
