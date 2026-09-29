// The general idea here to split buckets into shards by modulo:
// - shard 1 -> 0, 16, 32, ...
// - shard 2 -> 1, 17, 33, ...
// - shard 3 -> 2, 18, 34, ...
//
// We do not make extra-hot shart of first buckets,
// however, we still facing the hard-shard problem
// becuase the first buckets contain too much values
//
// We lock only the shard – certain buckets
void Striped_Lock_Collector::record(u64 request_delay_ms) {
    u64 bucket_index = std::min(request_delay_ms / _BUCKET_SPAN_MS, (u64)255);

    // get shard index
    u64 bucket_group_index = bucket_index % _NUM_BUCKET_GROUPS;

    // LOCK!
    {
        std::scoped_lock<std::mutex>
        lock(this->bucket_groups_mutexes[bucket_group_index]);

        this->buckets[bucket_index]++;
    }


    this->count.fetch_add(1);
    this->sum.fetch_add(request_delay_ms); 

    auto current_min = this->min.load();
    while (current_min > request_delay_ms
           && !this->min.compare_exchange_weak(current_min, request_delay_ms)) {};

    auto current_max = this->max.load();
    while (current_max < request_delay_ms
           && !this->max.compare_exchange_weak(current_max, request_delay_ms)) {};
}

// Returns approximated result, since the buckets could be changed after the copy
Snapshot Striped_Lock_Collector::get_snapshot() {
    
    std::array<u64, _NUM_BUCKETS> bucket_copies = {0};
    for (u64 bucket_group_index = 0;
         bucket_group_index < _NUM_BUCKET_GROUPS;
         bucket_group_index++) 
    {
        std::scoped_lock<std::mutex>
        lock(this->bucket_groups_mutexes[bucket_group_index]);

        for (u64 bucket_index = bucket_group_index;
             bucket_index < _NUM_BUCKETS;
             bucket_index += _NUM_BUCKET_GROUPS)
        {
            bucket_copies[bucket_index] = this->buckets[bucket_index];
        }
    }

    return {
        bucket_copies,
        this->count.load(),
        this->sum.load(),
        this->min.load(),
        this->max.load(),
        //p50 percentile
        utils__get_percentile(bucket_copies,
                              this->count.load(),
                              0.5),

        //p99 percentile
        utils__get_percentile(bucket_copies,
                              this->count.load(),
                              0.99),
    };
} 
