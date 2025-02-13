# compiler
CXX = riscv64-unknown-elf-g++
AS = riscv64-unknown-elf-gcc

# flags
CXXFLAGS = -march=rv64gcv -mabi=lp64d -O2 -Wall

# target
TARGET = rvv_test

# srcs
SRCS = main.cpp asm/strcpy.S asm/str_starts_with.S asm/str_starts_with_rvv.S asm/test_illegal_instructions.S

# objs
OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:.S=.o)

# build rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.S
	$(AS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)