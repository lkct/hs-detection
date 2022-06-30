SOURCE_DIR = ../../hs_detection/detect
OBJECT_DIR = build

# CXX = default
CPPFLAGS = -DNDEBUG -D_FORTIFY_SOURCE=2 -I$(SOURCE_DIR)
CXXFLAGS = -std=c++17 \
	-Wall -Wextra \
	-O3 -fwrapv -fstack-protector-strong -fopenmp \
	-march=native -mtune=native \
	-g
LDFLAGS = -fstack-protector-strong -fopenmp
LOADLIBES = 
LDLIBS = 

HEADERS = $(wildcard $(SOURCE_DIR)/*.h) $(wildcard $(SOURCE_DIR)/*/*.h)
SOURCES = main.cpp $(wildcard $(SOURCE_DIR)/[^d]*.cpp) $(wildcard $(SOURCE_DIR)/*/*.cpp)
OBJECTS = $(addprefix $(OBJECT_DIR)/,$(notdir $(SOURCES:.cpp=.o)))
ifeq ($(OS),Windows_NT)
	TARGET = main.exe
else
	TARGET = main
endif

VPATH = $(sort $(dir $(SOURCES)))

.PHONY: all clean asm

all: $(OBJECT_DIR)/$(TARGET)

clean:
	rm -f $(OBJECT_DIR)/*.o
	rm -f $(OBJECT_DIR)/$(TARGET)

$(OBJECT_DIR)/$(TARGET): $(OBJECTS) | $(OBJECT_DIR)
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(OBJECT_DIR)/%.o: %.cpp $(HEADERS) Makefile | $(OBJECT_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJECT_DIR):
	mkdir -p $(OBJECT_DIR)

asm: $(OBJECT_DIR)/Detection.s

$(OBJECT_DIR)/Detection.s: $(OBJECT_DIR)/Detection.o
	objdump -S -r -M intel --insn-width 8 $< > $@
