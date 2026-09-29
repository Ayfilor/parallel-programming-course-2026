Thread_Buffers* Double_Buffer_Collector::get_my_buffers() {

    struct TLSSlot {
        u64 id = 0;
        Thread_Buffers* state = nullptr;
    };
    static thread_local TLSSlot slot;

    if (slot.id != this->id) {
        auto state = std::make_unique<Thread_Buffers>();
        Thread_Buffers* raw_state = state.get();

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


void Double_Buffer_Collector::record(u64 request_delay_ms) {
    auto my_buffers = this->get_my_buffers();
    Selected_Buffer active_buffer;

    // Dekker Handshake:
    while (true) {
        // 1. which buffer is active?
        active_buffer = this->active_buffer.load();
        // 2. declare intention to write into active buffer
        my_buffers->inside.store(active_buffer);

        // 3. check if the reader changed the active buffer
        if (this->active_buffer.load() == active_buffer) break;

        // 4. if not, declare that won't write anywhere and iterate again
        my_buffers->inside.store(Selected_Buffer::Neither);
    }

    Thread_Buffer& selected_buffer = my_buffers->buf[active_buffer];
    u64 bucket_index = std::min(request_delay_ms / _BUCKET_SPAN_MS, (u64)255);
    
    // Record the request, just like in Single Threaded Approach!
    {
        selected_buffer.buckets[bucket_index]++;

        selected_buffer.count++;
        selected_buffer.sum += request_delay_ms; 

        selected_buffer.min = std::min(selected_buffer.min, request_delay_ms);
        selected_buffer.max = std::max(selected_buffer.max, request_delay_ms);
    }

    // Declare: we no longer use any buffer, we're done (with work)
    my_buffers->inside.store(Selected_Buffer::Neither);
}




Snapshot Double_Buffer_Collector::get_snapshot() {
    // LOCK: We lock the list of states (threads list in other works)
    std::lock_guard<std::mutex> lock(this->list_lock);

    // 1. Freeze the Active Buffer
    Selected_Buffer old_active = this->active_buffer.load();
    Selected_Buffer new_active = (old_active == Selected_Buffer::First)
                                    ? Selected_Buffer::Second
                                    : Selected_Buffer::First;

    // 2. Give the Writers the new buffer
    this->active_buffer.store(new_active);


    for (const auto& state : this->thread_states_list) {

        // 3. Is anybody write into the frozen buffer? 
        while (state->inside.load() == old_active) std::this_thread::yield();

        // 4. Migrate the state of considered buffer into the Global Buffer,
        // so the Global Buffer will be only in valid states of frozen buffers 
        for (u64 i = 0; i < _NUM_BUCKETS; ++i) {
            this->global_buffer.buckets[i] += state->buf[old_active].buckets[i];
        }

        this->global_buffer.count += state->buf[old_active].count;
        this->global_buffer.sum   += state->buf[old_active].sum;

        this->global_buffer.min 
            = std::min(this->global_buffer.min, state->buf[old_active].min);
        this->global_buffer.max 
            = std::max(this->global_buffer.max, state->buf[old_active].max);


        // 5. Since in working buffers we store only Delta between snapshots,
        // we clear the buffer – we accumulate the deltas of snapshots 
        metrics_collector__clear_buffer(&(state->buf[old_active]));
    }

    return {
        this->global_buffer.buckets,
        this->global_buffer.count,
        this->global_buffer.sum,
        this->global_buffer.min,
        this->global_buffer.max,
        utils__get_percentile(this->global_buffer.buckets,
                              this->global_buffer.count,
                              0.5),
        utils__get_percentile(this->global_buffer.buckets,
                              this->global_buffer.count,
                              0.99)
    };
}
