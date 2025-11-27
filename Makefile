# ------------------
# 变量定义
# ------------------

CC = gcc
CFLAGS = -Wall -g
LDFLAGS =

SRCDIR = src
BINDIR = bin
TESTDIR = test
# [新增] 定义测试源码文件的位置
TEST_SOURCE = tests/test.c

EXECUTABLE = tinyc

# [关键点] 这里只扫描 src 目录，所以 tests 目录下的文件不会被编译进编译器
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)%.c, $(BINDIR)/%.o, $(SOURCES))


# ------------------
# 规则定义
# ------------------

.PHONY: all
all: $(BINDIR)/$(EXECUTABLE)

$(BINDIR)/$(EXECUTABLE): $(OBJECTS)
	@$(CC) $(LDFLAGS) -o $@ $^

$(BINDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BINDIR)
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run
run: all
	@echo "--- Running Compiler (Default) ---"
	@./$(BINDIR)/$(EXECUTABLE)


# ------------------
# 测试规则
# ------------------

TEST_ASSEMBLY = $(TESTDIR)/output.s
TEST_EXECUTABLE = $(TESTDIR)/my_program
# [修改] 根据 tests/test.c 的逻辑，预期退出码应该是 23
EXPECTED_EXIT_CODE = 0

.PHONY: test
test: all
	@mkdir -p $(TESTDIR)
	@echo "--- Running Test on $(TEST_SOURCE) ---"
# 1. 编译 C 源码 -> 汇编文件
# [修改] 这里传入了 $(TEST_SOURCE) 作为参数，只有 make test 会读取这个文件
	@./$(BINDIR)/$(EXECUTABLE) $(TEST_SOURCE) > $(TEST_ASSEMBLY)
# 2. 汇编 -> 可执行文件 (去掉 -nostdlib 以支持 printf)
	@$(CC) $(TEST_ASSEMBLY) -o $(TEST_EXECUTABLE)
# 3. 加执行权限
	@chmod +x $(TEST_EXECUTABLE)
# 4. 运行程序 + 捕获退出码
	@./$(TEST_EXECUTABLE); \
	ACTUAL_EXIT_CODE=$$?; \
	if [ $$ACTUAL_EXIT_CODE -eq $(EXPECTED_EXIT_CODE) ]; then \
		echo "--- Test OK (Exit Code: $$ACTUAL_EXIT_CODE) ---"; \
	else \
		echo "--- Test FAILED (Expected $(EXPECTED_EXIT_CODE), got $$ACTUAL_EXIT_CODE) ---"; \
		exit 1; \
	fi
# 5. 清理 (调试时可以注释掉这一行查看 output.s)
	@rm -rf $(TESTDIR)

# ------------------
# 清理规则
# ------------------

.PHONY: clean
clean:
	@rm -rf $(BINDIR) $(TESTDIR) my_program output.s