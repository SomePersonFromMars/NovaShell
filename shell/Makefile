CC?=cc
DEBUG_FLAGS?=
DEFINES?=

# Automatically generate compile_commands.json
# for a language server.
BEAR?= 
ifdef BEAR
	CC:=bear --append -- $(CC)
endif

DEBUG?=
ifdef DEBUG
	DEBUG_FLAGS:=-g3
endif

DEBUGPRINT?=
ifdef DEBUGPRINT
	DEFINES:=$(DEFINES) DEBUGPRINT
endif

SRC_DIR=src
OBJ_DIR=obj
BIN_DIR=bin
INC_DIR=include

PARSE_DIR=input_parse

DEFINES:=$(addprefix -D,$(DEFINES))
CFLAGS=-I$(INC_DIR) $(DEBUG_FLAGS) $(DEFINES)


COMMON_SRCS=utils.c builtins.c executor.c inputhandler.c
SHELL_SRCS=$(COMMON_SRCS) mshell.c
TESTS_SRCS=$(COMMON_SRCS) tests.c
SRCS=$(COMMON_SRCS) mshell.c tests.c

COMMON_OBJS:=$(COMMON_SRCS:.c=.o)
COMMON_OBJS:=$(addprefix $(OBJ_DIR)/,$(COMMON_OBJS))
SHELL_OBJS:=$(SHELL_SRCS:.c=.o)
SHELL_OBJS:=$(addprefix $(OBJ_DIR)/,$(SHELL_OBJS))
TESTS_OBJS:=$(TESTS_SRCS:.c=.o)
TESTS_OBJS:=$(addprefix $(OBJ_DIR)/,$(TESTS_OBJS))
OBJS:=$(SRCS:.c=.o)
OBJS:=$(addprefix $(OBJ_DIR)/,$(OBJS))

.PHONY: all
all: check_dirs $(BIN_DIR)/mshell $(BIN_DIR)/tests

$(BIN_DIR)/mshell: $(SHELL_OBJS) $(OBJ_DIR)/siparse.a
	$(CC) $(CFLAGS) $(SHELL_OBJS) $(OBJ_DIR)/siparse.a -o $@ 

$(BIN_DIR)/tests: $(TESTS_OBJS) $(OBJ_DIR)/siparse.a
	$(CC) $(CFLAGS) $(TESTS_OBJS) $(OBJ_DIR)/siparse.a -o $@ 

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/siparse.a:
	$(MAKE) -C $(PARSE_DIR) INSTALL_DIR=$(realpath $(OBJ_DIR)) INC_DIR=$(realpath $(INC_DIR)) CC="$(CC)"

.PHONY: check_dirs
check_dirs:
	test -d $(OBJ_DIR) || mkdir $(OBJ_DIR)
	test -d $(BIN_DIR) || mkdir $(BIN_DIR)

.PHONY: clean
clean:
	rm -f $(BIN_DIR)/mshell $(OBJS) 2>/dev/null || true
	rm compile_commands.json 2>/dev/null || true

.PHONY: full_clean
full_clean: clean
	$(MAKE) -C $(PARSE_DIR) clean
	rm -f $(OBJ_DIR)/siparse.a
