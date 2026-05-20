BUILD_DIR = build
BIN_DIR = bin
SRC_DIR = src

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g 

TARGET_SEQ = $(BIN_DIR)/sequential
TARGET_PAR = $(BIN_DIR)/parallel
TARGETS = $(TARGET_SEQ) $(TARGET_PAR)

OBJS_SEQ = $(BUILD_DIR)/sequential.o
OBJS_PAR = $(BUILD_DIR)/parallel.o

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

all: $(TARGETS)

sequential: $(TARGET_SEQ)

parallel: $(TARGET_PAR)

$(TARGET_SEQ): $(OBJS_SEQ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_PAR): $(OBJS_PAR)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^)/parallel.o
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)

.PHONY: all clean
