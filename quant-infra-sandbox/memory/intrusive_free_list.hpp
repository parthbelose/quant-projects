//
// Created by parth on 04-06-2026.
//

#ifndef QUANT_INFRA_SANDBOX_INTRUSIVE_FREE_LIST_H
#define QUANT_INFRA_SANDBOX_INTRUSIVE_FREE_LIST_H

#endif //QUANT_INFRA_SANDBOX_INTRUSIVE_FREE_LIST_H
#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace quant_infra::memory {

    /**
     * @brief A zero-allocation, O(1) memory pool using an intrusive free list.
     * * @tparam T The type of object to store.
     * @tparam PoolSize The fixed maximum number of objects allocated at startup.
     */
    template <typename T, size_t PoolSize>
    class IntrusiveFreeList {
    private:
        // The union is the secret to zero memory overhead.
        // When active, it holds your object. When free, it acts as a linked list node.
        union Slot {
            T object;
            size_t next_free_index;

            // We explicitly define empty constructors/destructors because
            // we will manage the lifecycle of 'T' manually.
            Slot() {}
            ~Slot() {}
        };

        // The contiguous block of memory allocated once (on stack or heap).
        std::array<Slot, PoolSize> m_pool;

        // Pointer to the first available slot.
        size_t m_head_free_index;

        // Metrics tracking (optional, but good for debugging)
        size_t m_active_count;

    public:
        IntrusiveFreeList() : m_active_count(0) {
            // Initialize the free list chain
            for (size_t i = 0; i < PoolSize - 1; ++i) {
                m_pool[i].next_free_index = i + 1;
            }
            // Terminate the end of the list
            m_pool[PoolSize - 1].next_free_index = static_cast<size_t>(-1);
            m_head_free_index = 0;
        }

        // Variadic template forwards all arguments directly to T's constructor
        template <typename... Args>
        T* allocate(Args&&... args) {
            if (m_head_free_index == static_cast<size_t>(-1)) [[unlikely]] {
                throw std::bad_alloc(); // Pool is completely exhausted
            }

            // 1. Pop the head of the free list in O(1)
            size_t allocated_index = m_head_free_index;
            m_head_free_index = m_pool[allocated_index].next_free_index;

            // 2. Construct the object IN-PLACE (bypassing the OS allocator entirely)
            T* ptr = new (&m_pool[allocated_index].object) T(std::forward<Args>(args)...);

            m_active_count++;
            return ptr;
        }

        void deallocate(T* ptr) {
            if (!ptr) [[unlikely]] return;

            // 1. Manually destroy the object (since we didn't use standard 'delete')
            ptr->~T();

            // 2. Calculate exact array index via pointer arithmetic (O(1))
            Slot* slot = reinterpret_cast<Slot*>(ptr);
            size_t freed_index = std::distance(m_pool.data(), slot);

            // 3. Push this slot back onto the front of the free list
            slot->next_free_index = m_head_free_index;
            m_head_free_index = freed_index;

            m_active_count--;
        }

        // Utility function to check capacity
        size_t active_count() const { return m_active_count; }
        size_t capacity() const { return PoolSize; }
    };

} // namespace quant_infra::memory