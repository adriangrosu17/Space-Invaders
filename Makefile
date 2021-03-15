WIN ?= y
EXT ?= elf
DEBUG ?= y
CC = gcc
CXX = g++
TARGET_EXEC ?= SpaceInvaders.$(EXT)
BUILD_DIR ?= build
SRC_DIRS ?= src
SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
ifeq ($(WIN), y)
	SDL_DIR = SDL2-2.0.14
	TARGET_ARCH = x86_64
	INC_DIRS += $(SDL_DIR)/$(TARGET_ARCH)-w64-mingw32/include
	LDFLAGS += -L$(SDL_DIR)/$(TARGET_ARCH)-w64-mingw32/lib -lmingw32 -lSDL2main
	CC = $(TARGET_ARCH)-w64-mingw32-gcc
	CXX = $(TARGET_ARCH)-w64-mingw32-g++
	EXT = exe
endif

INC_DIRS += inc
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP
CFLAGS ?= -Wall -Wextra -Werror -pedantic -std=c99
ifeq ($(DEBUG), y)
	CFLAGS += -g
	CPPFLAGS += -DDEBUG
endif
LDFLAGS += -lSDL2 -lpthread -Xlinker -Map=$(BUILD_DIR)/$(TARGET_EXEC).map

# executable
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
ifeq ($(WIN), y)
	cp $(PWD)/$(SDL_DIR)/$(TARGET_ARCH)-w64-mingw32/bin/SDL2.dll $(PWD)/$(BUILD_DIR)/SDL2.dll
	cp /usr/$(TARGET_ARCH)-w64-mingw32/lib/libwinpthread-1.dll $(PWD)/$(BUILD_DIR)/libwinpthread-1.dll
endif
# assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)
-include $(DEPS)

MKDIR_P ?= mkdir -p

