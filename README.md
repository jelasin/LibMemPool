# LibMemPool C语言内存池

LibMemPool 是一个高性能、线程可选安全的 C 语言内存池库，支持自动链式扩容、固定大小类别、碎片整理与对齐分配。

## 🛠️ 快速开始

### 编译与运行

```bash
# 克隆仓库
git clone https://github.com/jelasin/LibMemPool.git
cd LibMemPool

# 编译静态/动态库
make

# 编译并运行内置示例与自检
make test

# 开启调试日志与断言（推荐用于定位问题）
make clean && make test DEBUG=1
```

### 基础使用

```c
#include "memory_pool.h"

int main() {
    // 创建16MB的线程安全内存池
    memory_pool_t* pool = memory_pool_create(16 * 1024 * 1024, true);
    
    // 分配内存
    void* ptr = memory_pool_alloc(pool, 1024);
    
    // 使用内存
    memset(ptr, 0, 1024);
    
    // 释放内存
    memory_pool_free(pool, ptr);
    
    // 销毁内存池
    memory_pool_destroy(pool);
    
    return 0;
}
```

## 📖 详细文档

### 内存池创建

```c
// 基础创建
memory_pool_t* pool = memory_pool_create(pool_size, thread_safe);

// 高级配置创建（可选）
pool_config_t config = {
    .pool_size = 64 * 1024 * 1024,
    .thread_safe = true,
    .alignment = 64,              // CPU缓存行对齐
    .enable_size_classes = true,
    .size_class_sizes = (size_t[]){64, 256, 1024, 4096},
    .num_size_classes = 4
};
memory_pool_t* pool = memory_pool_create_with_config(&config);

// 说明：
// - 内存不足时会自动创建“子池”，通过 pool->next 链接形成链式扩容；
// - memory_pool_destroy 会级联销毁整条链；
// - memory_pool_contains / validate / reset / defragment 会作用于整条链。
```

### 内存分配API

```c
// 基础分配
void* ptr = memory_pool_alloc(pool, size);

// 对齐分配（alignment 必须为2的幂）
void* aligned_ptr = memory_pool_alloc_aligned(pool, size, alignment);

// 零初始化分配
void* zero_ptr = memory_pool_calloc(pool, count, size);

// 重新分配
void* new_ptr = memory_pool_realloc(pool, old_ptr, new_size);

// 固定大小快速分配
void* fixed_ptr = memory_pool_alloc_fixed(pool, size);
```

### 内存释放

```c
// 普通释放
memory_pool_free(pool, ptr);

// 固定大小释放
memory_pool_free_fixed(pool, ptr);

// 重置整个池
memory_pool_reset(pool);
```

### 性能优化

```c
// 内存预热（减少页面错误）
memory_pool_warmup(pool);

// 碎片整理
memory_pool_defragment(pool);

// 添加固定大小类别
int class_id = memory_pool_add_size_class(pool, 1024, 1000);
```

### 调试与验证

```c
// 验证内存池完整性
bool is_valid = memory_pool_validate(pool);

// 检查指针是否属于池
bool contains = memory_pool_contains(pool, ptr);
```

#### 开启 DEBUG 宏

- 构建时指定 `MEMPOOL_DEBUG=1` 会启用调试日志与断言，示例：
- - `make clean && make test DEBUG=1`
- - 或 `make DEBUG=1`
- 相关宏在 `memory_pool.h`：
- - `MP_LOG(fmt, ...)` 条件日志
- - `MP_ASSERT(cond, msg)` 条件断言
- 关闭 DEBUG 时，这些宏在预处理阶段即被移除，不产生运行时开销（do { } while (0) 空展开）。

#### API 注意事项

- `alignment` 必须为 2 的幂，否则返回 `POOL_ERROR_INVALID_SIZE`。
- `memory_pool_alloc(pool, 0)` 会失败并设置 `POOL_ERROR_INVALID_SIZE`。
- 可通过 `memory_pool_get_last_error()` 与 `memory_pool_error_string()` 获取错误信息。

## 构建目标

- `make`：构建静态库 `lib/libmempool.a` 与动态库 `lib/libmempool.so`
- `make test`：编译并运行 `examples/examples.c`
- `make clean`：清理 `build/` 与 `lib/`

本项目采用 MIT 许可证。

---
