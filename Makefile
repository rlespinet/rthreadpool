SRC_DIR=src
OBJ_DIR=$(SRC_DIR)/obj

TST_SRC_DIR=tests
TST_OBJ_DIR=$(TST_SRC_DIR)/obj
TST_BIN_DIR=$(TST_SRC_DIR)/bin

SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
OBJ_FILES=$(addprefix $(OBJ_DIR)/,$(notdir $(SRC_FILES:.c=.o)))

TST_SRC_FILES=$(wildcard $(TST_SRC_DIR)/*.c)
TST_OBJ_FILES=$(addprefix $(TST_OBJ_DIR)/,$(notdir $(TST_SRC_FILES:.c=.o)))
TST_BIN_FILES=$(addprefix $(TST_BIN_DIR)/,$(notdir $(TST_SRC_FILES:.c=.out)))

CC=gcc
CFLAGS=-std=c11 -g -Wall -O0 -I $(SRC_DIR)
LDFLAGS=-pthread
OBJS=ia.o

all: directories tests

tests: ${TST_BIN_FILES}

directories: ${SRC_DIR} ${OBJ_DIR} ${TST_SRC_DIR} ${TST_OBJ_DIR} ${TST_BIN_DIR}

${SRC_DIR}:
	mkdir -p $@

${OBJ_DIR}:
	mkdir -p $@

${TST_SRC_DIR}:
	mkdir -p $@

${TST_OBJ_DIR}:
	mkdir -p $@

${TST_BIN_DIR}:
	mkdir -p $@

${TST_BIN_DIR}/%.out: $(TST_OBJ_DIR)/%.o $(OBJ_FILES)
	$(CC) -o $@ $^ $(LDFLAGS)

${TST_OBJ_DIR}/%.o: $(TST_SRC_DIR)/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

${OBJ_DIR}/%.o: $(SRC_DIR)/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm $(OBJ_DIR) $(TST_OBJ_DIR)

realclean: clean
	rm $(TST_BIN_FILES)
