# ------------------
# 变量定义
# ------------------

CC = gcc
CFLAGS = -Wall -g
LDFLAGS =

SRCDIR = src
BINDIR = bin
TESTDIR = test

EXECUTABLE = tinyc

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
	@./$(BINDIR)/$(EXECUTABLE)


# # ------------------
# # 测试规则
# # ------------------

# TEST_ASSEMBLY = $(TESTDIR)/output.s
# TEST_EXECUTABLE = $(TESTDIR)/my_program
# EXPECTED_EXIT_CODE = 5

# .PHONY: test
# test: all
# 	@mkdir -p $(TESTDIR)
# 	@echo "--- Running Test ---"
# # 1. 编译 C 源码 -> 汇编文件
# 	@./$(BINDIR)/$(EXECUTABLE) > $(TEST_ASSEMBLY)
# # 2. 汇编 -> 可执行文件
# 	@$(CC) -nostdlib $(TEST_ASSEMBLY) -o $(TEST_EXECUTABLE)
	
# # --- 核心修正：为生成的可执行文件添加执行权限 ---
# 	@chmod +x $(TEST_EXECUTABLE)
	
# # 3. 运行
# 	@./$(TEST_EXECUTABLE)
# # 4. 检查退出码
# 	@ACTUAL_EXIT_CODE=$$?; \
# 	if [ $$ACTUAL_EXIT_CODE -eq $(EXPECTED_EXIT_CODE) ]; then \
# 		echo "--- Test OK (Exit Code: $$ACTUAL_EXIT_CODE) ---"; \
# 	else \
# 		echo "--- Test FAILED (Expected $(EXPECTED_EXIT_CODE), got $$ACTUAL_EXIT_CODE) ---"; \
# 		exit 1; \
# 	fi
# # 5. 清理
# 	@rm -rf $(TESTDIR)

# ===================================================================
# 次测试代码不可运行，因为main返回值是5，非0，OS会识别错误导致测试终止
# 测试流程：
# make run
# ./bin/tinyc > output.s
# gcc -nostdlib output.s -o my_program
# chmod +x my_program
# ./my_program
# echo $?


# ------------------
# 清理规则
# ------------------

.PHONY: clean
clean:
	@rm -rf $(BINDIR) $(TESTDIR)