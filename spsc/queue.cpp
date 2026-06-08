#include <atomic>
#include <cstddef>
#include <iostream>
#include <new>
#include <thread>
#include <vector>

// ---------------------------------------------------------
// 1. The Queue Implementation
// ---------------------------------------------------------
template <typename T, size_t Capacity>
class SPSCQueue {
    static_assert((Capacity != 0) && ((Capacity & (Capacity - 1)) == 0), 
                  "Capacity must be a power of 2");

    static constexpr size_t CacheLineSize = 64;
    static constexpr size_t Mask = Capacity - 1;

    struct alignas(CacheLineSize) ProducerState {
        std::atomic<size_t> tail{0};
        size_t cached_head{0}; 
    };

    struct alignas(CacheLineSize) ConsumerState {
        std::atomic<size_t> head{0};
        size_t cached_tail{0};
    };

    ProducerState p_state;
    ConsumerState c_state;
    
    alignas(CacheLineSize) std::byte buffer[Capacity * sizeof(T)];

    T* get_slot(size_t index) noexcept {
        return reinterpret_cast<T*>(&buffer[(index & Mask) * sizeof(T)]);
    }

public:
    SPSCQueue() = default;

    ~SPSCQueue() {
        size_t head = c_state.head.load(std::memory_order_relaxed);
        size_t tail = p_state.tail.load(std::memory_order_relaxed);
        while (head < tail) {
            get_slot(head)->~T();
            head++;
        }
    }

    void* alloc_push() noexcept {
        size_t current_tail = p_state.tail.load(std::memory_order_relaxed);
        if (current_tail - p_state.cached_head == Capacity) {
            p_state.cached_head = c_state.head.load(std::memory_order_acquire);
            if (current_tail - p_state.cached_head == Capacity) return nullptr; 
        }
        return static_cast<void*>(get_slot(current_tail));
    }

    void commit_push() noexcept {
        size_t current_tail = p_state.tail.load(std::memory_order_relaxed);
        p_state.tail.store(current_tail + 1, std::memory_order_release);
    }

    T* peek_pop() noexcept {
        size_t current_head = c_state.head.load(std::memory_order_relaxed);
        if (current_head == c_state.cached_tail) {
            c_state.cached_tail = p_state.tail.load(std::memory_order_acquire);
            if (current_head == c_state.cached_tail) return nullptr;
        }
        return get_slot(current_head);
    }

    void commit_pop() noexcept {
        size_t current_head = c_state.head.load(std::memory_order_relaxed);
        get_slot(current_head)->~T();
        c_state.head.store(current_head + 1, std::memory_order_release);
    }

    template <typename... Args>
    bool emplace(Args&&... args) {
        void* slot = alloc_push();
        if (!slot) return false;
        ::new (slot) T(std::forward<Args>(args)...);
        commit_push();
        return true;
    }

    bool pop(T& out_item) {
        T* slot = peek_pop();
        if (!slot) return false;
        out_item = std::move(*slot);
        commit_pop();
        return true;
    }
};

// ---------------------------------------------------------
// 2. The Testing Ground
// ---------------------------------------------------------
int main() {
    // Create a queue with a capacity of 1024
    SPSCQueue<int, 1024> queue;
    const int messages_to_send = 100000;

    std::cout << "Starting SPSC Queue test..." << std::endl;

    // Producer Thread
    std::thread producer([&]() {
        for (int i = 0; i < messages_to_send; ++i) {
            // Keep trying if the queue is full
            while (!queue.emplace(i)) {
                // Yielding prevents locking up the CPU core if full
                std::this_thread::yield(); 
            }
        }
        std::cout << "Producer finished sending " << messages_to_send << " messages." << std::endl;
    });

    // Consumer Thread
    std::thread consumer([&]() {
        int received_value = 0;
        int count = 0;
        
        while (count < messages_to_send) {
            if (queue.pop(received_value)) {
                count++;
            } else {
                std::this_thread::yield();
            }
        }
        std::cout << "Consumer successfully received " << count << " messages." << std::endl;
        std::cout << "Final value received: " << received_value << std::endl;
    });

    // Wait for both threads to finish
    producer.join();
    consumer.join();

    std::cout << "Test completed successfully." << std::endl;
    return 0;
}