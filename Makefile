# SPDX-License-Identifier: TODO

CC = cc
CFLAGS = -Wall -Wextra -O2 -Iinclude
LDFLAGS =
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c,build/%.o,$(SRC))
BIN = cpm

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf build $(BIN) *.log
	$(MAKE) -C tests clean

install: $(BIN)
	mkdir -p $(HOME)/.cpm/bin
	cp $(BIN) $(HOME)/.cpm/bin/

test: clean cpm
	$(MAKE) -C tests > build_output.log 2>&1
	@cd tests && ./test_runner; \
	RET=$$?; \
	echo ""; \
	echo "  📊 CPM Code Coverage"; \
	lcov --capture --directory .. --output-file coverage.info > ../coverage_output.log 2>&1; \
	lcov --extract coverage.info "*/src/*" --output-file coverage.info >> ../coverage_output.log 2>&1; \
	lcov --summary coverage.info 2>&1 | grep -E "(lines|functions)" | sed 's/^//'; \
	exit $$RET
