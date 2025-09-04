# LibMemPool 简化Makefile

CC = gcc
# DEBUG=1 启用调试宏与更易调试的编译参数
ifeq ($(DEBUG),1)
	CFLAGS = -std=c99 -Wall -Wextra -O0 -g3 -fPIC -DMEMPOOL_DEBUG=1
else
	CFLAGS = -std=c99 -Wall -Wextra -O2 -g -fPIC
endif
LDFLAGS = -pthread
INCLUDES = -Iinclude

# 目录配置
SRCDIR = src
EXAMPLEDIR = examples
BUILDDIR = build
LIBDIR = lib

# 源文件
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# 库文件
STATIC_LIB = $(LIBDIR)/libmempool.a
SHARED_LIB = $(LIBDIR)/libmempool.so

# 默认目标
.PHONY: all clean test

all: $(STATIC_LIB) $(SHARED_LIB)

# 创建必要的目录
$(BUILDDIR) $(LIBDIR):
	@mkdir -p $@

# 编译目标文件
$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	@echo "编译 $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 静态库
$(STATIC_LIB): $(OBJECTS) | $(LIBDIR)
	@echo "创建静态库 $@"
	@ar rcs $@ $^

# 动态库
$(SHARED_LIB): $(OBJECTS) | $(LIBDIR)
	@echo "创建动态库 $@"
	@$(CC) -shared -o $@ $^ $(LDFLAGS)

# 编译并运行测试程序
test: $(STATIC_LIB) | $(BUILDDIR)
	@echo "编译示例程序..."
	@$(CC) $(CFLAGS) $(INCLUDES) $(EXAMPLEDIR)/examples.c $(STATIC_LIB) $(LDFLAGS) -o $(BUILDDIR)/examples
	@echo "运行示例..."
	@./$(BUILDDIR)/examples

# 清理构建文件
clean:
	@echo "清理构建文件..."
	@rm -rf $(BUILDDIR) $(LIBDIR)
