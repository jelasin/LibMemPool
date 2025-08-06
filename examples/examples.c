#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "../include/memory_pool.h"

// 获取当前时间（微秒）
static uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// 基础功能测试
void test_basic_functionality(void) {
    printf("=== 基础功能测试 ===\n");
    
    // 创建16MB内存池
    memory_pool_t* pool = memory_pool_create(16 * 1024 * 1024, true);
    assert(pool != NULL);
    
    // 测试分配
    void* ptr1 = memory_pool_alloc(pool, 1024);
    assert(ptr1 != NULL);
    printf("✓ 分配1024字节成功\n");
    
    void* ptr2 = memory_pool_alloc(pool, 2048);
    assert(ptr2 != NULL);
    printf("✓ 分配2048字节成功\n");
    
    // 测试写入数据
    memset(ptr1, 0xAA, 1024);
    memset(ptr2, 0xBB, 2048);
    printf("✓ 数据写入成功\n");
    
    // 验证数据
    assert(((char*)ptr1)[0] == (char)0xAA);
    assert(((char*)ptr2)[0] == (char)0xBB);
    printf("✓ 数据验证成功\n");
    
    // 测试释放
    memory_pool_free(pool, ptr1);
    memory_pool_free(pool, ptr2);
    printf("✓ 内存释放成功\n");
    
    // 测试calloc
    void* ptr3 = memory_pool_calloc(pool, 100, sizeof(int));
    assert(ptr3 != NULL);
    assert(((int*)ptr3)[0] == 0);
    printf("✓ calloc功能正常\n");
    
    memory_pool_free(pool, ptr3);
    
    // 测试realloc
    void* ptr4 = memory_pool_alloc(pool, 512);
    assert(ptr4 != NULL);
    memset(ptr4, 0xCC, 512);
    
    void* ptr5 = memory_pool_realloc(pool, ptr4, 1024);
    assert(ptr5 != NULL);
    assert(((char*)ptr5)[0] == (char)0xCC);
    printf("✓ realloc功能正常\n");
    
    memory_pool_free(pool, ptr5);
    
    // 验证内存池完整性
    assert(memory_pool_validate(pool));
    printf("✓ 内存池完整性验证通过\n");
    
    memory_pool_destroy(pool);
    printf("✓ 基础功能测试完成\n\n");
}

// 固定大小池测试
void test_fixed_size_pool(void) {
    printf("=== 固定大小池测试 ===\n");
    
    memory_pool_t* pool = memory_pool_create(16 * 1024 * 1024, true);
    assert(pool != NULL);
    
    // 添加固定大小类别
    int class64 = memory_pool_add_size_class(pool, 64, 1000);
    int class256 = memory_pool_add_size_class(pool, 256, 500);
    int class1024 = memory_pool_add_size_class(pool, 1024, 100);
    
    assert(class64 >= 0);
    assert(class256 >= 0);
    assert(class1024 >= 0);
    printf("✓ 固定大小类别添加成功\n");
    
    // 测试快速分配
    void* ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = memory_pool_alloc_fixed(pool, 64);
        assert(ptrs[i] != NULL);
    }
    printf("✓ 固定大小分配成功\n");
    
    // 测试快速释放
    for (int i = 0; i < 100; i++) {
        memory_pool_free_fixed(pool, ptrs[i]);
    }
    printf("✓ 固定大小释放成功\n");
    
    memory_pool_destroy(pool);
    printf("✓ 固定大小池测试完成\n\n");
}

// 性能测试
void test_performance(void) {
    printf("=== 性能测试 ===\n");
    
    const int iterations = 10000;
    size_t test_sizes[] = {32, 64, 128, 256, 512, 1024, 2048, 4096};
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    memory_pool_t* pool = memory_pool_create(64 * 1024 * 1024, true);
    assert(pool != NULL);
    
    // 预热内存池
    memory_pool_warmup(pool);
    
    // 内存池分配性能测试
    uint64_t start_time = get_timestamp_us();
    
    void* ptrs[iterations];
    for (int i = 0; i < iterations; i++) {
        size_t size = test_sizes[i % num_sizes];
        ptrs[i] = memory_pool_alloc(pool, size);
        if (!ptrs[i]) {
            printf("分配失败在第%d次\n", i);
            break;
        }
    }
    
    uint64_t alloc_time = get_timestamp_us() - start_time;
    
    start_time = get_timestamp_us();
    for (int i = 0; i < iterations; i++) {
        if (ptrs[i]) {
            memory_pool_free(pool, ptrs[i]);
        }
    }
    uint64_t free_time = get_timestamp_us() - start_time;
    
    printf("内存池性能:\n");
    printf("  分配 %d 次用时: %lu 微秒 (%.2f ns/次)\n", 
           iterations, alloc_time, (double)alloc_time * 1000 / iterations);
    printf("  释放 %d 次用时: %lu 微秒 (%.2f ns/次)\n", 
           iterations, free_time, (double)free_time * 1000 / iterations);
    
    // 系统malloc性能对比
    start_time = get_timestamp_us();
    for (int i = 0; i < iterations; i++) {
        size_t size = test_sizes[i % num_sizes];
        ptrs[i] = malloc(size);
    }
    uint64_t malloc_time = get_timestamp_us() - start_time;
    
    start_time = get_timestamp_us();
    for (int i = 0; i < iterations; i++) {
        if (ptrs[i]) {
            free(ptrs[i]);
        }
    }
    uint64_t system_free_time = get_timestamp_us() - start_time;
    
    printf("系统malloc性能:\n");
    printf("  分配 %d 次用时: %lu 微秒 (%.2f ns/次)\n", 
           iterations, malloc_time, (double)malloc_time * 1000 / iterations);
    printf("  释放 %d 次用时: %lu 微秒 (%.2f ns/次)\n", 
           iterations, system_free_time, (double)system_free_time * 1000 / iterations);
    
    double speedup_alloc = (double)malloc_time / alloc_time;
    double speedup_free = (double)system_free_time / free_time;
    
    printf("性能提升:\n");
    printf("  分配速度提升: %.2fx\n", speedup_alloc);
    printf("  释放速度提升: %.2fx\n", speedup_free);
    
    memory_pool_destroy(pool);
    printf("✓ 性能测试完成\n\n");
}

// 碎片化测试
void test_fragmentation(void) {
    printf("=== 碎片化测试 ===\n");
    
    memory_pool_t* pool = memory_pool_create(1024 * 1024, true);
    assert(pool != NULL);
    
    // 简单测试：分配3个连续的块，释放中间的
    void* ptr1 = memory_pool_alloc(pool, 128);
    void* ptr2 = memory_pool_alloc(pool, 128);
    void* ptr3 = memory_pool_alloc(pool, 128);
    
    printf("分配了3个块: ptr1=%p, ptr2=%p, ptr3=%p\n", ptr1, ptr2, ptr3);
    
    // 释放中间的块
    memory_pool_free(pool, ptr2);
    
    pool_stats_t stats1;
    memory_pool_get_stats(pool, &stats1);
    printf("释放ptr2后: 空闲块数量=%zu\n", stats1.free_block_count);
    
    // 释放第一个块 - 这应该与ptr2的块合并
    memory_pool_free(pool, ptr1);
    
    pool_stats_t stats2;
    memory_pool_get_stats(pool, &stats2);
    printf("释放ptr1后: 空闲块数量=%zu, 合并次数=%lu\n", stats2.free_block_count, stats2.merge_count);
    
    // 释放第三个块 - 这应该与前面合并的块再次合并
    memory_pool_free(pool, ptr3);
    
    pool_stats_t stats3;
    memory_pool_get_stats(pool, &stats3);
    printf("释放ptr3后: 空闲块数量=%zu, 合并次数=%lu\n", stats3.free_block_count, stats3.merge_count);
    
    // 现在做更大规模的测试
    printf("\n开始大规模碎片化测试...\n");
    
    // 分配固定大小的块，这样更容易产生相邻的碎片
    void* ptrs[100];  // 减少数量以便于观察
    for (int i = 0; i < 100; i++) {
        ptrs[i] = memory_pool_alloc(pool, 128);  // 固定大小
        if (!ptrs[i]) break;
    }
    
    // 释放一半（造成碎片）- 这样会产生相邻的空闲块
    for (int i = 0; i < 100; i += 2) {
        if (ptrs[i]) {
            memory_pool_free(pool, ptrs[i]);
            ptrs[i] = NULL;
        }
    }
    
    pool_stats_t stats_before;
    memory_pool_get_stats(pool, &stats_before);
    printf("碎片整理前: 空闲块数量=%zu, 碎片率=%zu%%, 合并次数=%lu\n", 
           stats_before.free_block_count, stats_before.fragmentation_ratio, stats_before.merge_count);
    
    // 执行碎片整理
    memory_pool_defragment(pool);
    
    pool_stats_t stats_after;
    memory_pool_get_stats(pool, &stats_after);
    printf("碎片整理后: 空闲块数量=%zu, 碎片率=%zu%%, 合并次数=%lu\n", 
           stats_after.free_block_count, stats_after.fragmentation_ratio, stats_after.merge_count);
    
    // 释放剩余内存
    for (int i = 1; i < 100; i += 2) {
        if (ptrs[i]) {
            memory_pool_free(pool, ptrs[i]);
        }
    }
    
    memory_pool_destroy(pool);
    printf("✓ 碎片化测试完成\n\n");
}

// 网络应用示例
void example_network_application(void) {
    printf("=== 网络应用示例 ===\n");
    
    // 模拟网络包结构
    typedef struct {
        uint32_t packet_id;
        uint16_t length;
        uint16_t protocol;
        char data[1500];  // 最大以太网帧
    } network_packet_t;
    
    // 创建优化的网络池
    pool_config_t config = {
        .pool_size = 32 * 1024 * 1024,
        .thread_safe = true,
        .alignment = 64,  // CPU缓存行对齐
        .enable_size_classes = true,
        .size_class_sizes = (size_t[]){sizeof(network_packet_t), 512, 1024, 2048},
        .num_size_classes = 4
    };
    
    memory_pool_t* net_pool = memory_pool_create_with_config(&config);
    if (!net_pool) {
        printf("内存池创建失败\n");
        return;
    }
    
    memory_pool_warmup(net_pool);
    printf("✓ 网络内存池创建成功\n");
    
    // 模拟高并发网络包处理
    const int packet_count = 5000;
    network_packet_t** packets = malloc(packet_count * sizeof(network_packet_t*));
    
    clock_t start = clock();
    
    // 快速分配网络包
    for (int i = 0; i < packet_count; i++) {
        packets[i] = (network_packet_t*)memory_pool_alloc_fixed(net_pool, sizeof(network_packet_t));
        if (packets[i]) {
            packets[i]->packet_id = i;
            packets[i]->length = 64 + (i % 1400);
            packets[i]->protocol = (i % 2) ? 6 : 17;  // TCP或UDP
            snprintf(packets[i]->data, sizeof(packets[i]->data), "Packet %d data", i);
        }
    }
    
    clock_t alloc_end = clock();
    
    // 模拟数据包处理
    int processed = 0;
    for (int i = 0; i < packet_count; i++) {
        if (packets[i]) {
            // 模拟防火墙检查
            if (packets[i]->protocol == 6 && packets[i]->length < 1000) {
                processed++;
            }
        }
    }
    
    // 快速释放网络包
    for (int i = 0; i < packet_count; i++) {
        if (packets[i]) {
            memory_pool_free_fixed(net_pool, packets[i]);
        }
    }
    
    clock_t free_end = clock();
    
    double alloc_time = (double)(alloc_end - start) / CLOCKS_PER_SEC * 1000;
    double free_time = (double)(free_end - alloc_end) / CLOCKS_PER_SEC * 1000;
    
    printf("网络包处理统计:\n");
    printf("  处理包数量: %d\n", packet_count);
    printf("  处理通过: %d\n", processed);
    printf("  分配用时: %.2f ms\n", alloc_time);
    printf("  释放用时: %.2f ms\n", free_time);
    printf("  总吞吐量: %.2f 包/秒\n", packet_count / ((alloc_time + free_time) / 1000));
    
    free(packets);
    memory_pool_destroy(net_pool);
    printf("✓ 网络应用示例完成\n\n");
}

// 线程安全测试数据
typedef struct {
    memory_pool_t* pool;
    int thread_id;
    int iterations;
    void** ptrs;
} thread_test_data_t;

// 线程测试函数
void* thread_test_func(void* arg) {
    thread_test_data_t* data = (thread_test_data_t*)arg;
    
    // 每个线程分配和释放内存
    for (int i = 0; i < data->iterations; i++) {
        size_t size = 64 + (i % 8) * 64;
        data->ptrs[i] = memory_pool_alloc(data->pool, size);
        
        if (data->ptrs[i]) {
            // 写入一些数据
            memset(data->ptrs[i], data->thread_id & 0xFF, size);
        }
        
        // 随机释放一些内存
        if (i > 10 && (i % 5) == 0) {
            int free_idx = i - 5;
            if (data->ptrs[free_idx]) {
                memory_pool_free(data->pool, data->ptrs[free_idx]);
                data->ptrs[free_idx] = NULL;
            }
        }
    }
    
    // 释放剩余内存
    for (int i = 0; i < data->iterations; i++) {
        if (data->ptrs[i]) {
            memory_pool_free(data->pool, data->ptrs[i]);
        }
    }
    
    return NULL;
}

// 多线程测试
void test_thread_safety(void) {
    printf("=== 多线程安全测试 ===\n");
    
    const int thread_count = 4;
    const int iterations_per_thread = 1000;
    
    memory_pool_t* pool = memory_pool_create(32 * 1024 * 1024, true);
    assert(pool != NULL);
    
    pthread_t threads[thread_count];
    thread_test_data_t thread_data[thread_count];
    
    // 创建线程
    for (int i = 0; i < thread_count; i++) {
        thread_data[i].pool = pool;
        thread_data[i].thread_id = i;
        thread_data[i].iterations = iterations_per_thread;
        thread_data[i].ptrs = malloc(iterations_per_thread * sizeof(void*));
        
        int ret = pthread_create(&threads[i], NULL, thread_test_func, &thread_data[i]);
        assert(ret == 0);
    }
    
    // 等待线程完成
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
        free(thread_data[i].ptrs);
    }
    
    // 验证内存池完整性
    assert(memory_pool_validate(pool));
    printf("✓ %d线程并发测试通过\n", thread_count);
    
    memory_pool_destroy(pool);
    printf("✓ 多线程安全测试完成\n\n");
}

// 压力测试
void test_stress(void) {
    printf("=== 压力测试 ===\n");
    
    memory_pool_t* pool = memory_pool_create(64 * 1024 * 1024, true);
    assert(pool != NULL);
    
    const int stress_iterations = 20000;
    void* ptrs[500];
    int ptr_count = 0;
    
    srand(time(NULL));
    
    for (int i = 0; i < stress_iterations; i++) {
        if (ptr_count < 500 && (rand() % 3) != 0) {
            // 70%概率分配
            size_t size = 16 + (rand() % 2048);
            ptrs[ptr_count] = memory_pool_alloc(pool, size);
            if (ptrs[ptr_count]) {
                ptr_count++;
            }
        } else if (ptr_count > 0) {
            // 30%概率释放
            int idx = rand() % ptr_count;
            memory_pool_free(pool, ptrs[idx]);
            ptrs[idx] = ptrs[ptr_count - 1];
            ptr_count--;
        }
        
        if (i % 5000 == 0) {
            printf("压力测试进度: %d/%d, 当前分配块数: %d\n", 
                   i, stress_iterations, ptr_count);
        }
    }
    
    // 释放剩余内存
    for (int i = 0; i < ptr_count; i++) {
        memory_pool_free(pool, ptrs[i]);
    }
    
    // 验证内存池完整性
    assert(memory_pool_validate(pool));
    printf("✓ 压力测试通过\n");
    
    memory_pool_destroy(pool);
    printf("✓ 压力测试完成\n\n");
}

// 内存池监控示例
void example_memory_monitoring(void) {
    printf("=== 内存池监控示例 ===\n");
    
    memory_pool_t* pool = memory_pool_create(16 * 1024 * 1024, true);
    
    // 分配各种大小的内存
    void* ptrs[500];
    for (int i = 0; i < 500; i++) {
        size_t size = 64 + (i % 10) * 64;
        ptrs[i] = memory_pool_alloc(pool, size);
    }
    
    // 显示详细统计信息
    printf("分配500个块后的状态:\n");
    memory_pool_print_stats(pool);
    
    // 释放一半内存（产生碎片）
    for (int i = 0; i < 500; i += 2) {
        if (ptrs[i]) {
            memory_pool_free(pool, ptrs[i]);
            ptrs[i] = NULL;
        }
    }
    
    printf("\n释放一半内存后的状态:\n");
    memory_pool_print_stats(pool);
    
    // 执行碎片整理
    memory_pool_defragment(pool);
    
    printf("\n碎片整理后的状态:\n");
    memory_pool_print_stats(pool);
    
    // 清理剩余内存
    for (int i = 1; i < 500; i += 2) {
        if (ptrs[i]) {
            memory_pool_free(pool, ptrs[i]);
        }
    }
    
    memory_pool_destroy(pool);
    printf("✓ 内存池监控示例完成\n\n");
}

int main(void) {
    printf("LibMemPool 高性能内存池测试和示例程序\n");
    printf("=========================================\n\n");
    
    // 运行所有测试
    test_basic_functionality();
    test_fixed_size_pool();
    test_performance();
    test_fragmentation();
    test_thread_safety();
    test_stress();
    
    // 运行应用示例
    example_network_application();
    example_memory_monitoring();
    
    printf("🎉 所有测试通过！LibMemPool运行正常。\n");
    printf("=========================================\n");
    
    return 0;
}
