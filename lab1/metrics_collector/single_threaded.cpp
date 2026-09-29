void Single_Threaded::record(u64 request_delay_ms) {
    u64 bucket_index = std::min(request_delay_ms / _BUCKET_SPAN_MS, (u64)255);
    
    this->buckets[bucket_index]++;
    this->count++;
    this->sum += request_delay_ms; 

    this->min = std::min(this->min, request_delay_ms);
    this->max = std::max(this->max, request_delay_ms);
}

Snapshot Single_Threaded::get_snapshot() {
    return {
        this->buckets,
        this->count,
        this->sum,
        this->min,
        this->max,
        //p50 percentile
        utils__get_percentile(this->buckets,
                              this->count,
                              0.5),

        //p99 percentile
        utils__get_percentile(this->buckets,
                              this->count,
                              0.99),
    };
} 
