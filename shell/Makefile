CC?=cc
# Automatically generate compile_commands.json
# for a language server.
BEAR?= 
ifdef BEAR
	CC:=bear --append -- $(CC)
endif

SRC_DIR=src
OBJ_DIR=obj
BIN_DIR=bin
INC_DIR=include

PARSERDIR=input_parse

CFLAGS=-I$(INC_DIR)


SRCS=utils.c mshell.c builtins.c
OBJS:=$(SRCS:.c=.o)
OBJS:=$(addprefix $(OBJ_DIR)/,$(OBJS))

all: check_dirs $(BIN_DIR)/mshell 

$(BIN_DIR)/mshell: $(OBJS) $(OBJ_DIR)/siparse.a
	$(CC) $(CFLAGS) $(OBJS) $(OBJ_DIR)/siparse.a -o $@ 

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/siparse.a:
	$(MAKE) -C $(PARSERDIR) INSTALL_DIR=$(realpath $(OBJ_DIR)) INC_DIR=$(realpath $(INC_DIR)) CC="$(CC)"

check_dirs:
	test -d $(OBJ_DIR) || mkdir $(OBJ_DIR)
	test -d $(BIN_DIR) || mkdir $(BIN_DIR)

clean:
	rm -f $(BIN_DIR)/mshell $(OBJS) 
	rm compile_commands.json

full_clean: clean
	$(MAKE) -C $(PARSERDIR) clean
	rm -f $(OBJ_DIR)/siparse.a
