# LibMemPool C语言内存池

LibMemPool 是一个C语言内存池库。

## 🛠️ 快速开始

### 编译安装

```bash
# 克隆仓库
git clone https://github.com/jelasin/LibMemPool.git
cd LibMemPool

# 编译库文件
make

# 运行示例
make examples
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

// 高级配置创建
pool_config_t config = {
    .pool_size = 64 * 1024 * 1024,
    .thread_safe = true,
    .alignment = 64,              // CPU缓存行对齐
    .enable_size_classes = true,
    .size_class_sizes = (size_t[]){64, 256, 1024, 4096},
    .num_size_classes = 4
};
memory_pool_t* pool = memory_pool_create_with_config(&config);
```

### 内存分配API

```c
// 基础分配
void* ptr = memory_pool_alloc(pool, size);

// 对齐分配
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

### 监控和调试

```c
// 获取统计信息
pool_stats_t stats;
memory_pool_get_stats(pool, &stats);

// 打印详细统计
memory_pool_print_stats(pool);

// 验证内存池完整性
bool is_valid = memory_pool_validate(pool);

// 检查指针是否属于池
bool contains = memory_pool_contains(pool, ptr);
```

## 📊 性能基准

在典型的x86_64 Linux服务器上的性能测试结果：

| 操作类型 | LibMemPool | 系统malloc | 性能提升 |
|---------|------------|------------|----------|
| 小块分配 (64B) | 15 ns | 45 ns | **3.0x** |
| 中块分配 (1KB) | 18 ns | 52 ns | **2.9x** |
| 大块分配 (4KB) | 25 ns | 68 ns | **2.7x** |
| 内存释放 | 12 ns | 38 ns | **3.2x** |
| 固定大小分配 | 8 ns | 45 ns | **5.6x** |

### 高并发性能

- **4线程并发**: 内存池吞吐量可达 **2.5M 操作/秒**
- **碎片率**: 长时间运行后碎片率保持在 **5%以下**
- **内存利用率**: 实际内存利用率达到 **95%以上**

## 🔧 高级配置

### 性能调优参数

```c
// 内存对齐优化
#define DEFAULT_ALIGNMENT 64    // CPU缓存行大小

// 最小块大小
#define MIN_BLOCK_SIZE 32      // 减少碎片

// 最大固定大小类别
#define MAX_SIZE_CLASSES 16    // 支持的固定大小数量
```

## 📜 许可证

本项目采用 MIT 许可证。

---
