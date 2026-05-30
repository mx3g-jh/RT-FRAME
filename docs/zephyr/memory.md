# 内存管理

Zephyr 提供三种内存分配方式：系统堆（通用）、自定义堆（隔离）、内存片（固定大小，零碎片）。
嵌入式场景优先用内存片，避免堆碎片。

---

## 全景

| 函数 / 宏 | 说明 | 典型用途 |
|-----------|------|---------|
| `k_malloc(size)` | 从系统堆分配 | 通用动态分配 |
| `k_free(ptr)` | 释放系统堆内存 | 配合 `k_malloc` |
| `k_calloc(nmemb, size)` | 分配并清零 | 需要零初始化的缓冲区 |
| `K_HEAP_DEFINE(name, size)` | 静态定义自定义堆 | 模块级隔离堆 |
| `k_heap_alloc(heap, size, timeout)` | 从自定义堆分配 | 隔离分配，支持超时 |
| `k_heap_free(heap, ptr)` | 释放自定义堆内存 | 配合 `k_heap_alloc` |
| `K_MEM_SLAB_DEFINE(name, size, num, align)` | 静态定义内存片池 | 固定大小，零碎片 |
| `k_mem_slab_init(slab, buf, size, num)` | 运行时初始化内存片池 | 动态场景 |
| `k_mem_slab_alloc(slab, mem, timeout)` | 分配一个内存片 | 实时分配，确定性延迟 |
| `k_mem_slab_free(slab, mem)` | 释放内存片 | 配合 `k_mem_slab_alloc` |
| `k_mem_slab_num_used_get(slab)` | 查询已用片数 | 状态监控 |
| `k_mem_slab_num_free_get(slab)` | 查询空闲片数 | 背压检测 |

**Kconfig 配置：**

| 配置项 | 说明 |
|--------|------|
| `CONFIG_HEAP_MEM_POOL_SIZE` | 系统堆大小（字节），0 表示禁用 |
| `CONFIG_SYS_HEAP_ALLOC_LOOPS` | 分配重试次数，影响碎片整理 |

---

## k_malloc / k_free — 系统堆

最简单的动态分配，适合大小不固定的场景。堆大小由 `CONFIG_HEAP_MEM_POOL_SIZE` 控制。

```c
#include <zephyr/kernel.h>

/* 分配 */
uint8_t *buf = k_malloc(256);
if (!buf) {
    LOG_ERR("alloc failed");
    return -ENOMEM;
}

/* 使用 */
memset(buf, 0, 256);

/* 释放 */
k_free(buf);
buf = NULL;
```

`k_calloc` 分配并清零：

```c
my_struct_t *obj = k_calloc(1, sizeof(my_struct_t));
```

项目用法（[src/middleware/uorb/uORB/uORBDeviceNode.cpp](../../src/middleware/uorb/uORB/uORBDeviceNode.cpp)）：
uORB DeviceNode 构造时分配循环队列缓冲区，析构时释放。

```cpp
_data = static_cast<uint8_t *>(k_malloc(meta->o_size * _queue_size));
if (!_data) {
    PX4_ERR("alloc failed for %s (%u bytes)", meta->o_name, meta->o_size * _queue_size);
}

/* 析构时 */
k_free(_data);
_data = nullptr;
```

---

## K_HEAP_DEFINE — 自定义堆

为模块创建独立堆，避免与系统堆竞争，分配失败不影响其他模块。

```c
/* 静态定义 4KB 自定义堆 */
K_HEAP_DEFINE(my_heap, 4096);

/* 分配（带超时） */
void *ptr = k_heap_alloc(&my_heap, 128, K_NO_WAIT);
if (!ptr) {
    LOG_ERR("heap alloc failed");
    return -ENOMEM;
}

/* 释放 */
k_heap_free(&my_heap, ptr);
```

---

## k_mem_slab — 内存片

固定大小块的池化分配，**无碎片，分配时间确定**，适合实时场景和频繁分配/释放的固定大小对象。

```c
/* 静态定义：每片 64 字节，共 8 片，4 字节对齐 */
K_MEM_SLAB_DEFINE(my_slab, 64, 8, 4);

/* 分配一片（永久等待直到有空闲片） */
void *mem;
if (k_mem_slab_alloc(&my_slab, &mem, K_FOREVER) == 0) {
    memset(mem, 0, 64);
    /* 使用 mem ... */
    k_mem_slab_free(&my_slab, mem);
} else {
    /* 池已满 */
}

/* 非阻塞分配（ISR 中使用） */
k_mem_slab_alloc(&my_slab, &mem, K_NO_WAIT);

/* 状态查询 */
uint32_t used = k_mem_slab_num_used_get(&my_slab);
uint32_t free = k_mem_slab_num_free_get(&my_slab);
```

典型模式：消息对象池。

```c
K_MEM_SLAB_DEFINE(msg_slab, sizeof(my_msg_t), 16, 4);

/* 发送方 */
my_msg_t *msg;
k_mem_slab_alloc(&msg_slab, (void **)&msg, K_FOREVER);
msg->value = 42;
k_msgq_put(&my_queue, &msg, K_FOREVER);  /* 传指针 */

/* 接收方 */
my_msg_t *msg;
k_msgq_get(&my_queue, &msg, K_FOREVER);
process(msg);
k_mem_slab_free(&msg_slab, msg);
```

---

## 选择指南

| 场景 | 用哪个 |
|------|--------|
| 大小不固定，偶尔分配 | `k_malloc` / `k_free` |
| 模块级隔离，避免影响系统堆 | `K_HEAP_DEFINE` + `k_heap_alloc` |
| 固定大小，频繁分配/释放，实时要求 | `k_mem_slab` |
| ISR 中分配 | `k_mem_slab`（`K_NO_WAIT`） |
| 不推荐 | 在中断中调用 `k_malloc`（可能阻塞） |
