// The main idea:
//    Each Thread gets its own `Thread_State` struct,
//    while Collector (which is global) stores list of states,
//    and on snapshot, thread should just traverse all the states (locking them)
Thread_State* Thread_Local_Collector::get_my_state() {

    struct TLSSlot {
        u64 id = 0;
        Thread_State* state = nullptr;
    };
    static thread_local TLSSlot slot;

    if (slot.id != this->id) {
        auto state = std::make_unique<Thread_State>();
        Thread_State* raw_state = state.get();

        // LOCK
        {
            std::lock_guard<std::mutex> lock(this->list_lock);
            this->thread_states_list.push_back(std::move(state));
        }

        slot.id = this->id;
        slot.state = raw_state;
    }
    return slot.state;
}


internal inline void 
relaxed_add(std::atomic<u64>& c, u64 delta) {
    c.store(c.load(std::memory_order_relaxed) + delta, std::memory_order_relaxed);
}


void Thread_Local_Collector::record(u64 request_delay_ms) {
    Thread_State* s = get_my_state();
    u64 b = std::min(request_delay_ms / 4, (u64)255);

    // Reader & Writer can work in the same buffer samultanusly,
    // so we use the Relaxed Memory Order – the only guarantee is atomicity,
    // other optimizations are allowed
    relaxed_add(s->buckets[b], 1);
    relaxed_add(s->count, 1);
    relaxed_add(s->sum, request_delay_ms);

    if (request_delay_ms < s->min.load(std::memory_order_relaxed))
        s->min.store(request_delay_ms, std::memory_order_relaxed);
    if (request_delay_ms > s->max.load(std::memory_order_relaxed))
        s->max.store(request_delay_ms, std::memory_order_relaxed);
}


Snapshot Thread_Local_Collector::get_snapshot() {

    std::array<u64, _NUM_BUCKETS> buckets{};
    u64 count = 0;
    u64 sum = 0;
    u64 min = _U64_UPPER_LIMIT;
    u64 max = 0;

    // LOCK:
    //   We lock the list of states (threads list in other works),
    //   not the values, so that we use Relaxed atomics instead of plain values
    {
        std::lock_guard<std::mutex> lock(this->list_lock);

        for (const auto& state : thread_states_list) {
            for (u64 i = 0; i < _NUM_BUCKETS; ++i) {
                buckets[i] += state->buckets[i].load(std::memory_order_relaxed);
            }

            count += state->count.load(std::memory_order_relaxed);
            sum   += state->sum.load(std::memory_order_relaxed);

            u64 s_min = state->min.load(std::memory_order_relaxed);
            if (s_min < min) min = s_min;

            u64 s_max = state->max.load(std::memory_order_relaxed);
            if (s_max > max) max = s_max;
        }
    }

    return {
        buckets, count, sum, min, max, 
        utils__get_percentile(buckets, count, 0.5),
        utils__get_percentile(buckets, count, 0.99),
    };
}
