CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -Wpedantic -g -lncurses -fsanitize=address -fsanitize=undefined
TARGET  = bigcalc
TEST    = test_bigcalc
BUILD   = build
SRC     = src

SRCS      = $(filter-out $(SRC)/test.c, $(wildcard $(SRC)/*.c))
TEST_SRCS = $(filter-out $(SRC)/main.c, $(wildcard $(SRC)/*.c))

OBJS      = $(patsubst $(SRC)/%.c, $(BUILD)/%.o, $(SRCS))
TEST_OBJS = $(patsubst $(SRC)/%.c, $(BUILD)/%.o, $(TEST_SRCS))

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

test: $(TEST)
	./$(TEST)

$(TEST): $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $(TEST) $(TEST_OBJS)

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD) $(TARGET) $(TEST)

.PHONY: test clean
