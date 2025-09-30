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
void* ptr = memory_pool_alloc(pool, size);

// Aligned allocation (alignment must be a power of 2)
void* aligned_ptr = memory_pool_alloc_aligned(pool, size, alignment);

// Zero-initialized allocation
void* zero_ptr = memory_pool_calloc(pool, count, size);

// Reallocation
void* new_ptr = memory_pool_realloc(pool, old_ptr, new_size);

// Fixed-size fast allocation
void* fixed_ptr = memory_pool_alloc_fixed(pool, size);
```

### Memory Freeing

```c
// Normal free
memory_pool_free(pool, ptr);

// Fixed-size free
memory_pool_free_fixed(pool, ptr);

// Reset entire pool
memory_pool_reset(pool);
```

### Performance Optimization

```c
// Memory warmup (reduce page faults)
memory_pool_warmup(pool);

// Defragmentation
memory_pool_defragment(pool);

// Add fixed-size class
int class_id = memory_pool_add_size_class(pool, 1024, 1000);
```

### Debugging and Validation

```c
// Validate memory pool integrity
bool is_valid = memory_pool_validate(pool);

// Check if pointer belongs to the pool
bool contains = memory_pool_contains(pool, ptr);
```

#### Enabling DEBUG Macro

- Specify `MEMPOOL_DEBUG=1` during build to enable debug logs and assertions, for example:
- - `make clean && make test DEBUG=1`
- - or `make DEBUG=1`
- Related macros in `memory_pool.h`:
- - `MP_LOG(fmt, ...)` conditional logging
- - `MP_ASSERT(cond, msg)` conditional assertion
- When DEBUG is disabled, these macros are removed during preprocessing, generating no runtime overhead (empty do { } while (0) expansion).

#### API Notes

- `alignment` must be a power of 2, otherwise returns `POOL_ERROR_INVALID_SIZE`.
- `memory_pool_alloc(pool, 0)` will fail and set `POOL_ERROR_INVALID_SIZE`.
- Error information can be obtained via `memory_pool_get_last_error()` and `memory_pool_error_string()`.

## Build Targets

- `make`: Builds static library `lib/libmempool.a` and dynamic library `lib/libmempool.so`
- `make test`: Compiles and runs `examples/examples.c`
- `make clean`: Cleans `build/` and `lib/`

This project uses the MIT License.

---
