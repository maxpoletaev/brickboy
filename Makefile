CC = clang
STD = -std=c17
WARNINGS = -Wall -Wextra -pedantic
IGNORED = -Wno-unused-parameter -Wno-gnu-zero-variadic-macro-arguments

.PHONY: all
all: tidy
	$(CC) -o brickboy $(STD) $(WARNINGS) $(IGNORED) -g -O0 src/*.c

.PHONY: tidy
tidy:
	clang-tidy src/*.c
