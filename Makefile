# ------------------
# 变量定义
# ------------------

# 编译器
CC = gcc
# 编译选项: -Wall (显示所有警告), -g (包含调试信息)
CFLAGS = -Wall -g
# 链接选项 (暂时为空)
LDFLAGS =

# 目录定义
SRCDIR = src
BINDIR = bin

# 可执行文件名
EXECUTABLE = tinyc

# 自动查找 src 目录下的所有 .c 文件
SOURCES = $(wildcard $(SRCDIR)/*.c)
# 根据源文件自动生成目标文件列表 (e.g., src/main.c -> bin/main.o)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(BINDIR)/%.o, $(SOURCES))


# ------------------
# 规则定义
# ------------------

# 默认目标: all
# 当你只输入 "make" 时，会执行这个目标
.PHONY: all
all: $(BINDIR)/$(EXECUTABLE)

# 链接规则: 如何从目标文件 (.o) 生成最终的可执行文件
$(BINDIR)/$(EXECUTABLE): $(OBJECTS)
	@echo "==> Linking..."
	@$(CC) $(LDFLAGS) -o $@ $^
	@echo "==> TinyC compiler is ready at $(BINDIR)/$(EXECUTABLE)"

# 编译规则: 如何从源文件 (.c) 生成目标文件 (.o)
# $@ 代表目标文件 (e.g., bin/main.o)
# $< 代表第一个依赖文件 (e.g., src/main.c)
$(BINDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BINDIR) # 如果 bin 目录不存在，则创建它
	@echo "==> Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# 运行目标
.PHONY: run
run: all
	@echo "==> Running TinyC..."
	@./$(BINDIR)/$(EXECUTABLE)

# 清理目标: 删除所有生成的文件
.PHONY: clean
clean:
	@echo "==> Cleaning up..."
	@rm -rf $(BINDIR)