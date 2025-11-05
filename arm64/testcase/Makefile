# 定义源文件目录和输出目录
SRC_DIR := test
OUTPUT_DIR := output

# 自动获取test目录下所有.c和.cpp源文件
SRCS := $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*.cpp)

# 生成对应的目标文件名（去掉路径和扩展名）
# 例如：test/hello.c → hello
TARGETS := $(patsubst $(SRC_DIR)/%.c,%,$(filter %.c,$(SRCS))) \
           $(patsubst $(SRC_DIR)/%.cpp,%,$(filter %.cpp,$(SRCS)))

# 定义输出文件路径（output/目标文件名）
OBJS := $(addprefix $(OUTPUT_DIR)/,$(TARGETS))

# 编译器和编译选项
CC := gcc
CXX := g++
CFLAGS := -Wall -O2  # C编译选项（-Wall显示警告，-O2优化）
CXXFLAGS := -Wall -O2  # C++编译选项

# 默认目标：编译所有源码
all: $(OBJS)

# 创建output目录（如果不存在）
$(OUTPUT_DIR):
	mkdir -p $@

# 编译C文件：将test/xxx.c编译为output/xxx
$(OUTPUT_DIR)/%: $(SRC_DIR)/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $< -o $@

# 编译C++文件：将test/xxx.cpp编译为output/xxx
$(OUTPUT_DIR)/%: $(SRC_DIR)/%.cpp | $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

# 清除output目录下的所有二进制文件
clean:
	rm -rf $(OUTPUT_DIR)/*

# 伪目标：避免与同名文件冲突
.PHONY: all clean
