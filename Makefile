
# The C compiler.
CC := gcc

# Executable name
EXE := busDB

# Where to find .c files
SRC := src

# Where to find .h files
HDR := includes

# Where to find .c test files (any .h exclusive for tests should also be in this
# directory)
TEST := tests

# Compilation flags
CFLAGS := -Wall -Werror

# Target build directory (will hold all binaries and .o files)
TARGET_DIR := target

# Build subdirectories
TEST_DIR := $(TARGET_DIR)/test
BUILD_DIR := $(TARGET_DIR)/build
DEBUG_DIR := $(TARGET_DIR)/debug
OBJ_DIR := $(TARGET_DIR)/obj

# Where the executable is sent to.
BIN := $(BUILD_DIR)/$(EXE)

# Where the debug executable is sent to.
DEBUG_BIN := $(DEBUG_DIR)/$(EXE)

# All .c files
SRCS := $(wildcard $(SRC)/*.c)

# All .c test files
TESTS := $(wildcard $(TEST)/*.c)

TEST_INCLUDE := src/utils.c

# All .h files
# NOTE: currently not used by any rules
HDRS := $(wildcard $(HDR)/*.h)

# All .o files (that must be generated)
OBJS := $(patsubst $(SRC)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# All executable tests
TEST_BINS := $(patsubst $(TEST)/%.c, $(TEST_DIR)/%, $(TESTS))

# Some colors!
ENDCOLOR := "\033[0m"
LINKCOLOR := "\033[4;32m"
CCCOLOR := "\033[4;34m"
TESTCOLOR := "\033[1;33m"
ERRORCOLOR := "\033[1;31m"
SUCCESSCOLOR := "\033[1;32m"
UNEDERLINE := "\033[4m"

# Printing make information
PRINT_COMPILE = printf '\t%b\t%b ---> %b\n' $(CCCOLOR)COMPILE$(ENDCOLOR) $(1) $(2)
PRINT_LINK = printf '\t%b\t%b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(1)
PRINT_TEST = printf '\t%b\t%b\n' $(TESTCOLOR)TESTING$(ENDCOLOR) $(1)
PRINT_ERROR = printf '\t%b\t%b\n' $(ERRORCOLOR)ERROR$(ENDCOLOR) $(1)
PRINT_SUCCESS = printf '\t%b\t%b\n' $(SUCCESSCOLOR)SUCCESS$(ENDCOLOR) $(1)

EVAL_TEST = ./$(1) > /dev/null 2>&1; \
			if [ $$? -eq 0 ]; \
				then \
					$(call PRINT_SUCCESS, "Test $(1)"); \
					echo "SUCCESS $(1)" >> /tmp/make-test; \
				else \
					$(call PRINT_ERROR, "Test $(1)"); \
					echo "FAIL $(1)" >> /tmp/make-test; \
			fi

all: $(BIN)

run: $(BIN)
	@./$<

debug: $(DEBUG_BIN)
	@./$<

$(DEBUG_BIN): CFLAGS := -g -DDEBUG $(CFLAGS)
$(DEBUG_BIN): $(OBJS) | $(DEBUG_DIR)
	@$(call PRINT_LINK, $@)
	@$(CC) -g $^ -o $@

test-setup:
	@rm -f /tmp/make-test
	@touch /tmp/make-test

test-teardown:
	@printf '\t%b tests ok!\n'     $$(cat /tmp/make-test | grep SUCCESS | wc -l)
	@printf '\t%b tests failed!\n' $$(cat /tmp/make-test | grep FAIL    | wc -l)
	@printf '\t%b tests total.\n'  $$(cat /tmp/make-test                | wc -l)
	# @rm /tmp/make-test

test: $(patsubst $(TEST)/%.c, test-run-one-%, $(TESTS))
	@echo '\t'$(UNEDERLINE)'Test results:'$(ENDCOLOR)
	@$(MAKE) -s test-teardown

test_%: $(TEST_DIR)/test_%
	@echo $(UNEDERLINE)'Test stdout/stderr:'$(ENDCOLOR)
	@$<

build_tests: $(patsubst $(TEST)/%.c, $(TEST_DIR)/%, $(TESTS))

# Prevent tests from beeing treated as intermidiate files
.PRECIOUS: $(TEST_DIR)/%

# Run a single test
test-run-one-%: $(TEST_DIR)/% | test-setup
	@$(call PRINT_TEST,$<)
	@$(call EVAL_TEST,$<)
	@echo # One blank line after each test

# Linking
$(BIN): $(OBJS) | $(BUILD_DIR)
	@$(call PRINT_LINK, $@)
	@$(CC) $^ -o $@

# Compiling to .o
$(OBJ_DIR)/%.o: $(SRC)/%.c | $(OBJ_DIR)
	@$(call PRINT_COMPILE, $<, $@)
	@$(CC) $(CFLAGS) -c $< -o $@ -I $(HDR)

.SECONDEXPANSION:
$(TEST_DIR)/test_%: $$(TEST)/test_%.c $$(wildcard $$(SRC)/%.c) | $(TEST_DIR)
	@$(call PRINT_COMPILE, $<, $@)
	@$(CC) $(CFLAGS) $^ $(TEST_INCLUDE) -o $@ -I $(TEST) -I $(HDR)

# Removes all make generated directories and files
clean:
	rm -rf $(TARGET_DIR)

# Creating directories
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
$(TEST_DIR):
	@mkdir -p $(TEST_DIR)
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
$(DEBUG_DIR):
	@mkdir -p $(DEBUG_DIR)

