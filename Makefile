ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif
SRC_DIR := server
INC_DIR := include
OBJ_DIR := build
LIB_DIR := lib

CC := gcc
AR := ar
CFLAGS_DEBUG := -Wall -Wextra -Werror -DNDEBUG -DHTTP_USE_MEMMEM -O2 -I$(INC_DIR)
CFLAGS_RELEASE := -Wall -Wextra -DDEBUG -DHTTP_USE_MEMMEM -g -O0 -I$(INC_DIR) 
ARFLAGS = rcs

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

LIB := $(LIB_DIR)/libloom.a

BUILD ?= release

ifeq ($(BUILD),debug)
	CFLAGS = $(CFLAGS_DEBUG)
else
	CFLAGS = $(CFLAGS_RELEASE)
endif

all: $(OBJ_DIR) $(LIB_DIR) $(LIB)

debug:
	$(MAKE) BUILD=debug

release:
	$(MAKE) BUILD=release

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(LIB_DIR): 
	mkdir -p $(LIB_DIR)

$(OBJ_DIR): 
	mkdir -p $(OBJ_DIR)

install: $(LIB)
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/include
	cp -f $(LIB) $(PREFIX)/lib/
	cp -rf $(INC_DIR)/* $(PREFIX)/include/

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)

.PHONY: all install clean
