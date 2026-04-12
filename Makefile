CC = gcc
CXX = g++
CFLAGS = -std=c11 -Wall -Wextra -g -I. -I./imgui $(shell pkg-config --cflags sdl2)
CXXFLAGS = -std=c++17 -Wall -Wextra -g -fpermissive -I. -I./imgui -I./imgui/backends $(shell pkg-config --cflags sdl2)
LDFLAGS = $(shell pkg-config --libs sdl2) -lGL -lX11

# Directories
BUILD_DIR = Debug
IMGUI_DIR = ./imgui

# Source files
SRC_C = main.c chip8.c texture.c sound.c
SRC_CPP = cimgui.cpp ./imgui/imgui.cpp ./imgui/imgui_draw.cpp ./imgui/imgui_widgets.cpp ./imgui/imgui_tables.cpp ./imgui/backends/imgui_impl_sdl2.cpp ./imgui/backends/imgui_impl_opengl2.cpp
ALL_SRC = $(SRC_C) $(SRC_CPP)

# Target
EXEC = $(BUILD_DIR)/ChimpC

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(ALL_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ALL_SRC) -o $(EXEC) $(LDFLAGS)
	@echo "Build complete: $@"

clean:
	@rm -f $(EXEC)
	@echo "Cleaned build artifacts"
