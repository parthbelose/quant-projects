
# Ultra-Low Latency SPSC Queue

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Architecture](https://img.shields.io/badge/Architecture-x86__64%20%7C%20ARM64-lightgrey)
![Performance](https://img.shields.io/badge/latency-sub--nanosecond-orange)

A highly optimized, production-grade Single-Producer Single-Consumer (SPSC) lock-free queue written in C++. Designed specifically for quantitative finance, high-frequency trading (HFT), and real-time systems where deterministic, ultra-low latency is critical.

This repository serves as both a functional header-only library and an educational resource on hardware-aware C++ design.

---

## 🧠 Core Theory & Architecture

Writing a fast queue isn't about algorithmic complexity (Big-O is $O(1)$ regardless). It is entirely about **mechanical sympathy**—understanding how the CPU interacts with memory.

### 1. The "False Sharing" Problem (Cache-Line Padding)
In modern CPUs, memory is not read byte-by-byte; it is fetched in chunks called **Cache Lines** (typically 64 bytes). 

If the Producer's `tail` index and the Consumer's `head` index sit next to each other in memory, they will load into the *same* cache line. When the Producer updates the `tail`, the CPU invalidates that entire cache line across all cores. The Consumer, trying to read the `head`, suffers a catastrophic cache miss and must wait hundreds of cycles to fetch the memory from RAM again. This is called "False Sharing."

**The Solution:** We force the compiler to pad these variables so they sit on separate cache lines using `alignas(64)`.
```cpp
struct alignas(64) ProducerState { std::atomic<size_t> tail; };
struct alignas(64) ConsumerState { std::atomic<size_t> head; };

```

### 2. Amortizing Atomic Reads (Locally Cached Indices)

Atomic operations prevent race conditions, but atomic *reads* across CPU cores are slow because they require synchronizing the L1 caches via the MESI protocol.

Instead of the Producer reading the Consumer's atomic `head` on every single push, the Producer keeps a *local, non-atomic* copy of the head (`cached_head`).

* The Producer pushes blindly, incrementing the tail.
* Only when the Producer *thinks* the queue is full (because `tail - cached_head == Capacity`) does it perform the expensive atomic read of the true `head` to see if the Consumer has freed up space.

This reduces cross-core synchronization traffic by orders of magnitude.

### 3. Memory Ordering (Release / Acquire Semantics)

By default, C++ atomics use `std::memory_order_seq_cst` (Sequential Consistency), which emits heavy CPU fence instructions. We do not need this strictness.

We use **Acquire-Release semantics**:

* **`memory_order_release` (Producer):** Ensures that all the data written to the queue slot is actually visible in memory *before* the `tail` index is incremented.
* **`memory_order_acquire` (Consumer):** Ensures that reading the `tail` index happens *before* reading the data in the queue slot.

This prevents the CPU's out-of-order execution engine from reading garbage data.

### 4. Power-of-Two Masking

A ring buffer must wrap around when it reaches its maximum capacity. The standard mathematical approach is modulo arithmetic: `index % Capacity`.
However, integer division is notoriously slow on CPUs (often taking 15-20 cycles).

By forcing the queue capacity to be a power of two (e.g., 1024, 2048), we can replace the modulo operation with a bitwise AND, which takes 1 CPU cycle:

```cpp
// If Capacity is 1024, Mask is 1023 (binary: 0000001111111111)
size_t slot_index = current_head & (Capacity - 1); 

```

### 5. Zero-Copy & Uninitialized Storage

In high-throughput systems, copying data into the queue and then copying it out is too expensive.

* The buffer is allocated as raw, uninitialized memory (`std::byte array`).
* The **Pointer API** (`alloc_push` / `peek_pop`) allows threads to access the memory slots directly. Objects are constructed directly in the ring buffer using placement `new`, meaning a network packet can be parsed straight into the queue without intermediate heap allocations or copies.

---

## 🚀 Usage

Because this is a single-header implementation, integrating and testing it requires no complex build systems.

### The Single-File Sandbox

For isolated testing or working inside a containerized Linux environment via SSH, compile the provided `spsc.cpp` directly:

```bash
# Compile with C++20, maximum optimizations (-O3), and threading support
g++ -std=c++20 -O3 -pthread spsc.cpp -o spsc_test

# Execute
./spsc_test

```

### API Examples

**1. The Emplace / Value API (Standard Usage)**
Clean and memory-safe for internal logging and configuration passing.

```cpp
SPSCQueue<int, 1024> queue;

// Producer Thread
queue.emplace(42);

// Consumer Thread
int data;
if (queue.pop(data)) {
    // Process data
}

```

**2. The Zero-Copy API (HFT / Network Feed Handlers)**
Used when bypassing intermediate memory copies is necessary.

```cpp
struct TradeTick { double price; int qty; };
SPSCQueue<TradeTick, 2048> queue;

// Producer Thread
void* slot = queue.alloc_push();
if (slot) {
    // Construct directly in queue memory
    TradeTick* tick = new (slot) TradeTick{150.25, 100}; 
    queue.commit_push();
}

// Consumer Thread
TradeTick* tick = queue.peek_pop();
if (tick) {
    // Read directly from queue memory
    std::cout << tick->price << std::endl;
    queue.commit_pop(); // Destroys object and frees slot
}

```