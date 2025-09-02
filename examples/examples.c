// Clean, comprehensive example and tests for LibMemPool
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "../include/memory_pool.h"

#define KB(x) ((size_t)(x) * 1024)
#define MB(x) ((size_t)(x) * 1024 * 1024)

static void test_basic(void) {
    printf("[basic] 开始\n");
    memory_pool_t* pool = memory_pool_create(MB(16), true);
    assert(pool);

    // alloc/free
    void* a = memory_pool_alloc(pool, 1024);
    void* b = memory_pool_alloc(pool, 2048);
    assert(a && b);
    memset(a, 0xAA, 1024);
    memset(b, 0xBB, 2048);
    memory_pool_free(pool, a);
    memory_pool_free(pool, b);

    // calloc
    int* c = (int*)memory_pool_calloc(pool, 100, sizeof(int));
    assert(c && c[0] == 0);
    memory_pool_free(pool, c);

    // realloc growth
    char* d = (char*)memory_pool_alloc(pool, 512);
    assert(d);
    memset(d, 0xCC, 512);
    char* d2 = (char*)memory_pool_realloc(pool, d, 1536);
    assert(d2 && d2[0] == (char)0xCC);
    memory_pool_free(pool, d2);

    // aligned alloc
    void* e = memory_pool_alloc_aligned(pool, 1000, 128);
    assert(e && ((uintptr_t)e % 128 == 0));
    memory_pool_free(pool, e);

    // invalid size
    void* z = memory_pool_alloc(pool, 0);
    assert(z == NULL && memory_pool_get_last_error() == POOL_ERROR_INVALID_SIZE);
    // invalid pointer free
    void* bad = (void*)0x12345;
    memory_pool_free(pool, bad);
    assert(memory_pool_get_last_error() == POOL_ERROR_INVALID_POINTER);

    // validate and reset
    assert(memory_pool_validate(pool));
    memory_pool_reset(pool);
    assert(memory_pool_validate(pool));

    memory_pool_destroy(pool);
    printf("[basic] 通过\n");
}

static void test_fixed_classes(void) {
    printf("[fixed] 开始\n");
    memory_pool_t* pool = memory_pool_create(MB(16), true);
    assert(pool);

    int c64 = memory_pool_add_size_class(pool, 64, 1000);
    int c256 = memory_pool_add_size_class(pool, 256, 400);
    int c1k = memory_pool_add_size_class(pool, 1024, 100);
    assert(c64 >= 0 && c256 >= 0 && c1k >= 0);

    void* ptrs[300];
    for (int i = 0; i < 300; ++i) {
        size_t sz = (i % 3 == 0) ? 64 : (i % 3 == 1) ? 256 : 1024;
        ptrs[i] = memory_pool_alloc_fixed(pool, sz);
        assert(ptrs[i]);
        memset(ptrs[i], 0x5A, sz);
    }
    for (int i = 0; i < 300; ++i) {
        memory_pool_free_fixed(pool, ptrs[i]);
    }

    assert(memory_pool_validate(pool));
    memory_pool_destroy(pool);
    printf("[fixed] 通过\n");
}

static void test_fragmentation_defrag(void) {
    printf("[frag] 开始\n");
    memory_pool_t* pool = memory_pool_create(MB(2), true);
    assert(pool);

    enum { N = 200 };
    void* v[N];
    for (int i = 0; i < N; ++i) {
        v[i] = memory_pool_alloc(pool, 256);
        assert(v[i]);
    }
    for (int i = 0; i < N; i += 2) {
        memory_pool_free(pool, v[i]);
        v[i] = NULL;
    }
    // defragment and allocate a larger block to ensure merging works
    memory_pool_defragment(pool);
    void* big = memory_pool_alloc(pool, 256 * 50);
    assert(big);
    memory_pool_free(pool, big);

    for (int i = 1; i < N; i += 2) {
        memory_pool_free(pool, v[i]);
    }
    assert(memory_pool_validate(pool));
    memory_pool_destroy(pool);
    printf("[frag] 通过\n");
}

static void test_chain_growth(void) {
    printf("[chain] 开始\n");
    memory_pool_t* pool = memory_pool_create(KB(64), true);
    assert(pool);

    size_t need = KB(96);
    void* p = memory_pool_alloc(pool, need);
    assert(p);
    // expect a child pool created
    assert(pool->next != NULL);
    assert(memory_pool_contains(pool, p));
    memory_pool_free(pool, p);

    void* blocks[100]; int i = 0;
    for (; i < 100; ++i) {
        blocks[i] = memory_pool_alloc(pool, 256);
        if (!blocks[i]) break;
    }
    void* extra = memory_pool_alloc(pool, 256);
    assert(extra != NULL);
    assert(pool->next != NULL);
    for (int j = 0; j < i; ++j) if (blocks[j]) memory_pool_free(pool, blocks[j]);
    memory_pool_free(pool, extra);

    assert(memory_pool_validate(pool));
    memory_pool_destroy(pool);
    printf("[chain] 通过\n");
}

typedef struct {
    memory_pool_t* pool;
    int id;
    int iters;
    unsigned seed;
} worker_arg_t;

static void* worker(void* argp) {
    worker_arg_t* arg = (worker_arg_t*)argp;
    void** bag = (void**)malloc(sizeof(void*) * 1024);
    int count = 0;
    for (int i = 0; i < arg->iters; ++i) {
        if ((rand_r(&arg->seed) & 3) != 0) {
            size_t sz = 32 + (rand_r(&arg->seed) % 2048);
            void* p = memory_pool_alloc(arg->pool, sz);
            if (p) {
                if (count < 1024) bag[count++] = p; else memory_pool_free(arg->pool, p);
            }
        } else if (count > 0) {
            int idx = rand_r(&arg->seed) % count;
            memory_pool_free(arg->pool, bag[idx]);
            bag[idx] = bag[count - 1];
            --count;
        }
    }
    for (int i = 0; i < count; ++i) memory_pool_free(arg->pool, bag[i]);
    free(bag);
    return NULL;
}

static void test_multithread(void) {
    printf("[mt] 开始\n");
    memory_pool_t* pool = memory_pool_create(MB(32), true);
    assert(pool);
    const int T = 4;
    pthread_t th[T]; worker_arg_t args[T];
    for (int i = 0; i < T; ++i) {
        args[i].pool = pool; args[i].id = i; args[i].iters = 5000; args[i].seed = (unsigned)time(NULL) ^ (i * 7919);
        assert(pthread_create(&th[i], NULL, worker, &args[i]) == 0);
    }
    for (int i = 0; i < T; ++i) pthread_join(th[i], NULL);
    assert(memory_pool_validate(pool));
    memory_pool_destroy(pool);
    printf("[mt] 通过\n");
}

static void test_warmup_and_aligned_errors(void) {
    printf("[misc] 开始\n");
    memory_pool_t* pool = memory_pool_create(MB(8), true);
    assert(pool);
    memory_pool_warmup(pool);
    // non power-of-two alignment should fail
    void* x = memory_pool_alloc_aligned(pool, 64, 24);
    assert(x == NULL && memory_pool_get_last_error() == POOL_ERROR_INVALID_SIZE);
    assert(memory_pool_validate(pool));
    memory_pool_destroy(pool);
    printf("[misc] 通过\n");
}

int main(void) {
    printf("LibMemPool 全面示例与测试\n");
    printf("========================\n");
    test_basic();
    test_fixed_classes();
    test_fragmentation_defrag();
    test_chain_growth();
    test_multithread();
    test_warmup_and_aligned_errors();
    printf("全部通过\n");
    return 0;
}
