# Compiler and flags
CC := gcc
AR := ar
RM := rm -f

# Directories
SRC_DIR := src
TEST_DIR := test
BUILD_DIR := build
LIB_DIR := lib
INCLUDE_DIR := include

# Source directories
LEXER_DIR := $(SRC_DIR)/lexer
PARSER_DIR := $(SRC_DIR)/parser
VM_DIR := $(SRC_DIR)/vm
COMPILER_DIR := $(SRC_DIR)/compiler

# Collect all source files
SRCS := $(wildcard $(SRC_DIR)/*.c) \
        $(wildcard $(LEXER_DIR)/*.c) \
        $(wildcard $(PARSER_DIR)/*.c) \
        $(wildcard $(VM_DIR)/*.c) \
        $(wildcard $(COMPILER_DIR)/*.c)

# Object files
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

# Test sources and objects
TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS := $(TEST_SRCS:%.c=$(BUILD_DIR)/%.o)
TEST_BINS := $(TEST_SRCS:%.c=$(BUILD_DIR)/%)

# Library name
LIB_NAME := libosfl
STATIC_LIB := $(LIB_DIR)/$(LIB_NAME).a
SHARED_LIB := $(LIB_DIR)/$(LIB_NAME).so

# Compiler flags
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic
CFLAGS += -I$(INCLUDE_DIR) -I$(SRC_DIR)
CFLAGS += -fPIC -MMD -MP

# Debug flags
DEBUG_FLAGS := -g -DDEBUG
SANITIZE_FLAGS := -fsanitize=address,undefined

# Release flags
RELEASE_FLAGS := -O2 -DNDEBUG

# Default to debug build
CFLAGS += $(DEBUG_FLAGS)

# Dependencies
DEPS := $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)

# Platform detection
ifeq ($(OS),Windows_NT)
    SHARED_LIB_EXT := dll
    RM := del /Q
    MKDIR := mkdir
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        SHARED_LIB_EXT := dylib
    else
        SHARED_LIB_EXT := so
    endif
    MKDIR := mkdir -p
endif

# Default target
.PHONY: all
all: dirs $(STATIC_LIB) $(SHARED_LIB)

# Create necessary directories
.PHONY: dirs
dirs:
    @$(MKDIR) $(BUILD_DIR) $(BUILD_DIR)/$(SRC_DIR) $(BUILD_DIR)/$(TEST_DIR) \
              $(BUILD_DIR)/$(LEXER_DIR) $(BUILD_DIR)/$(PARSER_DIR) \
              $(BUILD_DIR)/$(VM_DIR) $(BUILD_DIR)/$(COMPILER_DIR) \
              $(LIB_DIR)

# Debug build
.PHONY: debug
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

# Release build
.PHONY: release
release: CFLAGS += $(RELEASE_FLAGS)
release: all

# Sanitizer build
.PHONY: sanitize
sanitize: CFLAGS += $(DEBUG_FLAGS) $(SANITIZE_FLAGS)
sanitize: all

# Static library
$(STATIC_LIB): $(OBJS)
    @echo "Creating static library..."
    $(AR) rcs $@ $^

# Shared library
$(SHARED_LIB): $(OBJS)
    @echo "Creating shared library..."
    $(CC) -shared -o $@ $^

# Compile source files
$(BUILD_DIR)/%.o: %.c
    @echo "Compiling $<..."
    @$(MKDIR) $(dir $@)
    $(CC) $(CFLAGS) -c $< -o $@

# Build and run tests
.PHONY: test
test: CFLAGS += $(DEBUG_FLAGS)
test: $(TEST_BINS)
    @echo "Running tests..."
    @for test in $(TEST_BINS) ; do \
        echo "Running $$test..." ; \
        $$test ; \
    done

# Build test executables
$(BUILD_DIR)/$(TEST_DIR)/%: $(BUILD_DIR)/$(TEST_DIR)/%.o $(STATIC_LIB)
    @echo "Building test $@..."
    $(CC) $(CFLAGS) $^ -o $@

# Clean build files
.PHONY: clean
clean:
    @echo "Cleaning..."
    $(RM) -r $(BUILD_DIR)/* $(LIB_DIR)/*

# Very clean (including dependency files)
.PHONY: distclean
distclean: clean
    $(RM) $(DEPS)

# Install
.PHONY: install
install: all
    @echo "Installing..."
    install -d $(DESTDIR)/usr/local/lib
    install -m 644 $(STATIC_LIB) $(DESTDIR)/usr/local/lib/
    install -m 755 $(SHARED_LIB) $(DESTDIR)/usr/local/lib/
    install -d $(DESTDIR)/usr/local/include
    install -m 644 $(INCLUDE_DIR)/*.h $(DESTDIR)/usr/local/include/

# Uninstall
.PHONY: uninstall
uninstall:
    @echo "Uninstalling..."
    $(RM) $(DESTDIR)/usr/local/lib/$(notdir $(STATIC_LIB))
    $(RM) $(DESTDIR)/usr/local/lib/$(notdir $(SHARED_LIB))
    $(RM) $(DESTDIR)/usr/local/include/osfl.h

# Include dependencies
-include $(DEPS)