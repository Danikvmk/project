#!/bin/bash

BIN_DIR := bin
SRC_DIR := source
INC_DIR := include
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/object
DEP_DIR := $(BUILD_DIR)/dependency

TARGET := $(BIN_DIR)/interpreter

SRC_EXT := cpp

SRCS := $(wildcard $(SRC_DIR)/*.$(SRC_EXT))
OBJS := $(patsubst $(SRC_DIR)/%.$(SRC_EXT), $(OBJ_DIR)/%.o, $(SRCS))
DEPS := $(patsubst $(SRC_DIR)/%.$(SRC_EXT), $(DEP_DIR)/%.d, $(SRCS))

CC := g++
LD := g++

CFLAGS := -g -Wall -Wextra -std=c++23
CPPFLAGS := -I$(INC_DIR)
DEPFLAGS = -MMD -MT $@ -MF $(DEP_DIR)/$*.d
LDFLAGS :=

all: $(TARGET)

#Linking
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(LD) $(LDFLAGS) $^ -o $@

#Compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.$(SRC_EXT) | $(OBJ_DIR) $(DEP_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@ $(DEPFLAGS)


$(OBJ_DIR) $(DEP_DIR) $(BIN_DIR):
	@mkdir -p $@

-include $(DEPS)

run: $(TARGET)
	@$<

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean run