# LibMemPool C Memory Pool

LibMemPool is a high-performance, thread-safe, memory-safe C memory pool library that supports automatic chain expansion, fixed-size classes, defragmentation, and aligned allocation.

## 🛠️ Quick Start

### Compile and Run

```bash
# Clone the repository
git clone https://github.com/jelasin/LibMemPool.git
cd LibMemPool

# Compile static/dynamic library
make

# Compile and run built-in examples and self-check
make test

# Enable debug logs and assertions (recommended for troubleshooting)
make clean && make test DEBUG=1
```

### Basic Usage

```c
#include "memory_pool.h"

int main() {
    // Create a 16MB thread-safe memory pool
    memory_pool_t* pool = memory_pool_create(16 * 1024 * 1024, true);
    
    // Allocate memory
    void* ptr = memory_pool_alloc(pool, 1024);
    
    // Use memory
    memset(ptr, 0, 1024);
    
    // Free memory
    memory_pool_free(pool, ptr);
    
    // Destroy the memory pool
    memory_pool_destroy(pool);
    
    return 0;
}
```

## 📖 Detailed Documentation

### Memory Pool Creation

```c
// Basic creation
memory_pool_t* pool = memory_pool_create(pool_size, thread_safe);

// Advanced configuration creation (optional)
pool_config_t config = {
    .pool_size = 64 * 1024 * 1024,
    .thread_safe = true,
    .alignment = 64,              // CPU cache line alignment
    .enable_size_classes = true,
    .size_class_sizes = (size_t[]){64, 256, 1024, 4096},
    .num_size_classes = 4
};
memory_pool_t* pool = memory_pool_create_with_config(&config);

// Notes:
// - Automatically creates "child pools" via pool->next links for chain expansion when memory is insufficient;
// - memory_pool_destroy cascades destruction to the entire chain;
// - memory_pool_contains / validate / reset / defragment operate on the entire chain.
```

### Memory Allocation API

```c
// Basic allocation
void
