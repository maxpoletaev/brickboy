CC = clang
STD = -std=c17
WARNINGS = -Wall -Wextra -Wconversion -Werror -pedantic
IGNORED = -Wno-gnu-zero-variadic-macro-arguments

.PHONY: all
all: tidy
	@echo "--------- running: $@ ---------"
	$(CC) -o brickboy $(STD) $(WARNINGS) $(IGNORED) -g -O0 src/*.c

.PHONY: tidy
tidy:
	@echo "--------- running: $@ ---------"
	clang-tidy src/*.c --

.PHONY: clean
clean:
	@echo "--------- running: $@ ---------"
	rm -f brickboy
