BUILD_DIR = build
BIN_DIR = bin
SRC_DIR = src
CC = mpicc
RUN = mpirun
TEST_ARGS ?= --hostfile mpi_hosts
RUN_ARGS ?= --hostfile mpi_hosts
TEST_GRAPH = graph.bin 1965206 2769244
DEBUG_GRAPH = dados_entrada_sequencial 1965206 2769244
DEBUG_PAR_LOG = debug_parallel.log
DEBUG_SEQ_LOG = debug_sequential.log
GRAPH = data/setup_cod_paralelos/graph.bin 10000000 800000000
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2
MPIRUN_FLAGS = --mca btl_tcp_if_include 172.26.1.0/24
DEBUG_DIR = debug

TARGET_SEQ = $(BIN_DIR)/sequential
TARGET_SEQ_NO_FILTER = $(BIN_DIR)/sequential_no_filter
TARGET_PAR = $(BIN_DIR)/parallel
TARGET_SEQ_DEBUG = $(BIN_DIR)/sequential_debug
TARGET_PAR_DEBUG = $(BIN_DIR)/parallel_debug
TARGETS = $(TARGET_SEQ) $(TARGET_SEQ_NO_FILTER) $(TARGET_PAR)

OBJS_SEQ = $(BUILD_DIR)/sequential.o \
           $(BUILD_DIR)/core/graph.o \
           $(BUILD_DIR)/core/disjointset.o \
           $(BUILD_DIR)/core/mst_output.o \
           $(BUILD_DIR)/mst/mst.o

OBJS_SEQ_NO_FILTER = $(BUILD_DIR)/sequential_no_filter.o \
                     $(BUILD_DIR)/core/graph.o \
                     $(BUILD_DIR)/core/disjointset.o \
                     $(BUILD_DIR)/core/mst_output.o \
                     $(BUILD_DIR)/mst/mst.o \
                     $(BUILD_DIR)/mst/mst_no_filter.o

OBJS_PAR = $(BUILD_DIR)/parallel.o \
           $(BUILD_DIR)/core/graph.o \
           $(BUILD_DIR)/core/disjointset.o \
           $(BUILD_DIR)/core/mst_output.o \
           $(BUILD_DIR)/mst/mst.o \
           $(BUILD_DIR)/mpi/mst_mpi.o \
           $(BUILD_DIR)/mpi/types.o

OBJS_SEQ_DEBUG = $(BUILD_DIR)/$(DEBUG_DIR)/sequential_debug.o \
                 $(BUILD_DIR)/$(DEBUG_DIR)/mst_debug.o \
                 $(BUILD_DIR)/core/graph.o \
                 $(BUILD_DIR)/core/disjointset.o \
                 $(BUILD_DIR)/core/mst_output.o

OBJS_PAR_DEBUG = $(BUILD_DIR)/$(DEBUG_DIR)/parallel_debug.o \
                 $(BUILD_DIR)/$(DEBUG_DIR)/mst_mpi_debug.o \
                 $(BUILD_DIR)/core/graph.o \
                 $(BUILD_DIR)/core/disjointset.o \
                 $(BUILD_DIR)/core/mst_output.o \
                 $(BUILD_DIR)/mpi/types.o

all: $(TARGETS)

sequential: $(TARGET_SEQ)
sequential_no_filter: $(TARGET_SEQ_NO_FILTER)
parallel: $(TARGET_PAR)
sequential_debug: $(TARGET_SEQ_DEBUG)
parallel_debug: $(TARGET_PAR_DEBUG)

$(TARGET_SEQ): $(OBJS_SEQ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_SEQ_NO_FILTER): $(OBJS_SEQ_NO_FILTER)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_PAR): $(OBJS_PAR)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_SEQ_DEBUG): $(OBJS_SEQ_DEBUG)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_PAR_DEBUG): $(OBJS_PAR_DEBUG)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(DEBUG_DIR)/%.o: $(DEBUG_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGETS)
	$(RUN) $(MPIRUN_FLAGS) $(TEST_ARGS) ./$(TARGET_PAR) $(TEST_GRAPH)
	./$(TARGET_SEQ) $(TEST_GRAPH)

run: $(TARGETS)
	$(RUN) $(MPIRUN_FLAGS) $(RUN_ARGS) ./$(TARGET_PAR) $(GRAPH)
	./$(TARGET_SEQ) $(GRAPH)

debug: $(TARGET_PAR_DEBUG) $(TARGET_SEQ_DEBUG)
	$(RUN) -np 4 ./$(TARGET_PAR_DEBUG) $(DEBUG_GRAPH) > $(DEBUG_PAR_LOG) 2>&1
	./$(TARGET_SEQ_DEBUG) $(DEBUG_GRAPH) > $(DEBUG_SEQ_LOG) 2>&1

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)

.PHONY: all sequential sequential_no_filter parallel sequential_debug parallel_debug test run debug clean
