// helper.cpp
std::vector<u64> 
generate_zipf_values(u64 N,
                     u64 K,
                     f64 exponent,
                     u32 seed);

void
print_total(const std::vector<u64>& values);

void
print_head_and_tail(const std::vector<u64>& values);




// percentile.cpp
u64 utils__get_percentile(
    const std::array<u64, _NUM_BUCKETS> buckets,
    u64 records_count,
    f64 percentile);
