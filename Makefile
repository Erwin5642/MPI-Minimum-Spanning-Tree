BUILD_DIR = build
BIN_DIR = bin
SRC_DIR = src
CC = mpicc
RUN = mpirun
TEST_ARGS ?= -np 4
RUN_ARGS ?= -np 4
TEST_GRAPH = graph.bin 1965206 2769244
GRAPH = data/setup_cod_paralelos/graph.bin 10000000 800000000
CFLAGS = -Wall -Wextra -pedantic -g -O2

TARGET_SEQ = $(BIN_DIR)/sequential
TARGET_PAR = $(BIN_DIR)/parallel
TARGETS = $(TARGET_SEQ) $(TARGET_PAR)

OBJS_SEQ = $(BUILD_DIR)/sequential.o \
           $(BUILD_DIR)/core/graph.o \
           $(BUILD_DIR)/core/disjointset.o \
           $(BUILD_DIR)/core/mst.o \
           $(BUILD_DIR)/io/stream.o

OBJS_PAR = $(BUILD_DIR)/parallel.o \
           $(BUILD_DIR)/core/graph.o \
           $(BUILD_DIR)/core/disjointset.o \
           $(BUILD_DIR)/core/mst.o \
           $(BUILD_DIR)/core/mst_mpi.o \
           $(BUILD_DIR)/io/stream.o \
           $(BUILD_DIR)/mpi/types.o

all: $(TARGETS)

sequential: $(TARGET_SEQ)
parallel: $(TARGET_PAR)

$(TARGET_SEQ): $(OBJS_SEQ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_PAR): $(OBJS_PAR)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGETS)
	$(RUN) $(MPIRUN_FLAGS) $(TEST_ARGS) ./$(TARGET_PAR) $(TEST_GRAPH)
	./$(TARGET_SEQ) $(TEST_GRAPH)

run: $(TARGETS)
	$(RUN) $(MPIRUN_FLAGS) $(RUN_ARGS) ./$(TARGET_PAR) $(GRAPH)
	./$(TARGET_SEQ) $(GRAPH)

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)

.PHONY: all sequential parallel test run clean
