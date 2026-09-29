// Get percentile by cumulative search
u64 utils__get_percentile(
    const std::array<u64, _NUM_BUCKETS> buckets, // FIXME: use reference, don't copy
    u64 records_count,
    f64 percentile) 
{
    if (records_count == 0) return 0;

    f64 threshold = records_count * percentile;

    u64 accumulated = 0;
    for (u64 i = 0; i < _NUM_BUCKETS; i++) {
        accumulated += buckets[i];
        if (accumulated >= threshold) {
            return i * _BUCKET_SPAN_MS;
        }
    }

    return (_NUM_BUCKETS - 1) * _BUCKET_SPAN_MS;
}
