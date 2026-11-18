# Makefile - Dots and Boxes (CMPS 241, Sprint 1)
#
#   make            build the game  -> ./dotsboxes
#   make run        build and start the game
#   make test       build and run unit tests + scripted full-game tests
#   make valgrind   run a full scripted game under Valgrind
#   make install    copy the game to /usr/local/bin (run as root)
#   make clean      remove all build output

CC      = cc
# -g     : debug symbols for GDB and readable Valgrind reports
# -O0    : no optimisation, so GDB shows every variable and line in order
# -MMD -MP : generate .d files so editing a .h rebuilds the right .c files
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -g -O0 -Iinclude -MMD -MP
LDFLAGS =

TARGET    = dotsboxes
TEST_BIN  = test_board
BUILD_DIR = build
PREFIX    = /usr/local

# Every .c file in src/ except main.c is shared by the game and the tests.
LIB_SRCS  = src/board.c src/display.c src/input.c src/game.c
LIB_OBJS  = $(LIB_SRCS:src/%.c=$(BUILD_DIR)/%.o)
MAIN_OBJ  = $(BUILD_DIR)/main.o
TEST_OBJ  = $(BUILD_DIR)/test_board.o
DEPS      = $(LIB_OBJS:.o=.d) $(MAIN_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

VALGRIND_FLAGS = --leak-check=full --show-leak-kinds=all \
                 --track-origins=yes --error-exitcode=1

.PHONY: all run test valgrind install clean

all: $(TARGET)

$(TARGET): $(LIB_OBJS) $(MAIN_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(TEST_BIN): $(LIB_OBJS) $(TEST_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_board.o: tests/test_board.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET) $(TEST_BIN)
	./$(TEST_BIN)
	sh tests/run_tests.sh

valgrind: $(TARGET)
	valgrind $(VALGRIND_FLAGS) ./$(TARGET) < tests/inputs/tie_game.txt > /dev/null

install: $(TARGET)
	install -m 755 $(TARGET) $(PREFIX)/bin/$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_BIN)

-include $(DEPS)
