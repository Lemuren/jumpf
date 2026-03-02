# Compiler and flags.
CC       := gcc
CFLAGS   := -std=c11 -Wall -Wextra -Wpedantic -g
LDFLAGS  := 
LDLIBS   := -lsqlite3 -lm

# Directories.
SRC_DIR      := src
DB_DIR       := $(SRC_DIR)/db
UTIL_DIR     := $(SRC_DIR)/util
JUMPF_DIR    := $(SRC_DIR)/jumpf
JUMPF_D_DIR  := $(SRC_DIR)/jumpfd
TEST_DIR     := tests
BUILD_DIR    := build
OBJ_DIR      := $(BUILD_DIR)/obj
BIN_DIR      := $(BUILD_DIR)/bin

# Binaries.
JUMPF_BIN    := $(BIN_DIR)/jumpf
JUMPF_D_BIN  := $(BIN_DIR)/jumpfd
TEST_BIN     := $(BIN_DIR)/tests

# Source files.
DB_SRC       := $(DB_DIR)/db.c
UTIL_SRC     := $(UTIL_DIR)/config.c $(UTIL_DIR)/path.c
JUMPF_SRC    := $(JUMPF_DIR)/main.c $(JUMPF_DIR)/cli.c
JUMPF_D_SRC  := $(JUMPF_D_DIR)/main.c $(JUMPF_D_DIR)/fanotify.c $(JUMPF_D_DIR)/filters.c
TEST_SRC     := $(TEST_DIR)/test_db.c

# Object files.
DB_OBJ       := $(DB_SRC:%.c=$(OBJ_DIR)/%.o)
UTIL_OBJ     := $(UTIL_SRC:%.c=$(OBJ_DIR)/%.o)
JUMPF_OBJ    := $(JUMPF_SRC:%.c=$(OBJ_DIR)/%.o)
JUMPF_D_OBJ  := $(JUMPF_D_SRC:%.c=$(OBJ_DIR)/%.o)
TEST_OBJ     := $(TEST_SRC:%.c=$(OBJ_DIR)/%.o)

# Default target.
all: $(JUMPF_BIN) $(JUMPF_D_BIN)

# Build rules for binaries.
$(JUMPF_BIN): $(DB_OBJ) $(UTIL_OBJ) $(JUMPF_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(JUMPF_D_BIN): $(DB_OBJ) $(UTIL_OBJ) $(JUMPF_D_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# Test binary.
$(TEST_BIN): $(DB_OBJ) $(UTIL_OBJ) $(TEST_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) -lcmocka

# Run tests.
test: $(TEST_BIN)
	$(TEST_BIN)

# Pattern rule for object files.
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

# Clean.
clean:
	@rm -rf $(BUILD_DIR)

# Help.
help:
	@echo "make all\tbuild jumpf and jumpfd"
	@echo "make test\trun unit tests"
	@echo "make clean\tclean build artifacts"

.PHONY: all clean test help

