#pragma once

#include <map>
#include <cstdint>
#include <algorithm>
#include <iostream>

namespace quant_infra::matching {

    struct PriceLevel {
        uint64_t price;
        uint64_t total_volume;
        Order* head;
        Order* tail;

        PriceLevel(uint64_t p) : price(p), total_volume(0), head(nullptr), tail(nullptr) {}

        void append_order(Order* order) {
            if (!head) {
                head = tail = order;
            } else {
                tail->next = order;
                order->prev = tail;
                tail = order;
            }
            total_volume += order->quantity;
        }

        void remove_order(Order* order) {
            if (order->prev) order->prev->next = order->next;
            if (order->next) order->next->prev = order->prev;
            if (head == order) head = order->next;
            if (tail == order) tail = order->prev;

            order->next = nullptr;
            order->prev = nullptr;
            total_volume -= order->quantity;
        }
    };

    // Templated to accept your specific IntrusiveFreeList type
    template <typename MemoryPool>
    class LimitOrderBook {
    private:
        std::map<uint64_t, PriceLevel, std::greater<uint64_t>> m_bids;
        std::map<uint64_t, PriceLevel, std::less<uint64_t>> m_asks;

        // Pointer to our zero-allocation memory pool
        MemoryPool* m_pool;

        void add_order(Order* order) {
            if (order->is_buy) {
                auto [it, inserted] = m_bids.try_emplace(order->price_ticks, order->price_ticks);
                it->second.append_order(order);
            } else {
                auto [it, inserted] = m_asks.try_emplace(order->price_ticks, order->price_ticks);
                it->second.append_order(order);
            }
        }

    public:
        // Pass the pool during initialization
        explicit LimitOrderBook(MemoryPool* pool) : m_pool(pool) {}

        void process_order(Order* aggressive_order) {
            if (aggressive_order->is_buy) {
                // Matching a Buy against resting Asks
                while (aggressive_order->quantity > 0 && !m_asks.empty()) {
                    auto best_ask_it = m_asks.begin();

                    // If the best ask is higher than what the buyer is willing to pay, stop matching.
                    if (best_ask_it->first > aggressive_order->price_ticks) {
                        break;
                    }

                    PriceLevel& level = best_ask_it->second;
                    Order* resting_order = level.head;

                    while (resting_order && aggressive_order->quantity > 0) {
                        uint32_t fill_qty = std::min(aggressive_order->quantity, resting_order->quantity);

                        std::cout << "[TRADE EVENT] Matched " << fill_qty
                                  << " shares @ " << resting_order->price_ticks << std::endl;

                        aggressive_order->quantity -= fill_qty;
                        resting_order->quantity -= fill_qty;

                        Order* next_order = resting_order->next;

                        // If the resting order is fully filled, remove and deallocate it
                        if (resting_order->quantity == 0) {
                            level.remove_order(resting_order);
                            m_pool->deallocate(resting_order);
                        }
                        resting_order = next_order;
                    }

                    // Clean up the price level if it's empty to keep the map fast
                    if (level.head == nullptr) {
                        m_asks.erase(best_ask_it);
                    }
                }
            } else {
                // Matching a Sell against resting Bids
                while (aggressive_order->quantity > 0 && !m_bids.empty()) {
                    auto best_bid_it = m_bids.begin();

                    if (best_bid_it->first < aggressive_order->price_ticks) {
                        break;
                    }

                    PriceLevel& level = best_bid_it->second;
                    Order* resting_order = level.head;

                    while (resting_order && aggressive_order->quantity > 0) {
                        uint32_t fill_qty = std::min(aggressive_order->quantity, resting_order->quantity);

                        std::cout << "[TRADE EVENT] Matched " << fill_qty
                                  << " shares @ " << resting_order->price_ticks << std::endl;

                        aggressive_order->quantity -= fill_qty;
                        resting_order->quantity -= fill_qty;

                        Order* next_order = resting_order->next;

                        if (resting_order->quantity == 0) {
                            level.remove_order(resting_order);
                            m_pool->deallocate(resting_order);
                        }
                        resting_order = next_order;
                    }

                    if (level.head == nullptr) {
                        m_bids.erase(best_bid_it);
                    }
                }
            }

            // If the aggressive order still has remaining quantity, it rests in the book
            if (aggressive_order->quantity > 0) {
                add_order(aggressive_order);
            } else {
                // Fully filled aggressive order, return it to the pool
                m_pool->deallocate(aggressive_order);
            }
        }

        void print_top_of_book() const {
            uint64_t best_bid = m_bids.empty() ? 0 : m_bids.begin()->first;
            uint64_t best_ask = m_asks.empty() ? 0 : m_asks.begin()->first;
            std::cout << "Spread: Bid " << best_bid << " | Ask " << best_ask << std::endl;
        }
    };

} // namespace quant_infra::matching