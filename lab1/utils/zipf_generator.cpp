#define ZIPF_RANGE (1 << 20)


// Generate N Zipf-distributed values in the [0; K) range.
//
// The idea is generete probability weights 
// of possible output numbers by Zipf's PDF (Density),
// make them cummulative to treat them as ranges
// where we can map the uniformely distributed numbers,
// and map the output to the requested K values
//
// ZIPF_RANGE limits the supported number of output values.
//
// The general algorithm description is:
// 1. Generate cumulative weights of Zipf probabilities
// 2. Map uniformly distributed numbers to Zipf by weights
std::vector<u64> 
generate_zipf_values(u64 N,
                     u64 K,
                     f64 exponent,
                     u32 seed)
{
    assert(K > 0 && K <= ZIPF_RANGE,
           "bucket count is outside the supported range",
           "K", K, "ZIPF_RANGE", ZIPF_RANGE);

    std::vector<u64> values; values.reserve(N);
    std::vector<f64> cumulative_weights(K);

    // Generate weights. Make them cumulative, so we can treat them as ranges
    // and map the uniformly distributed number to a number according to our weights.
    // The Zipf PDF (Density) is $P(i) = 1 / i^a$ with an $a$ parameter
    f64 total_weight = 0.0;
    for (u64 i = 0; i < K; i++) {
        total_weight += 1.0 / std::pow(i + 1, exponent);
        cumulative_weights[i] = total_weight;
    }

    // Generate N random numbers and transform them according to the weights.
    std::mt19937 random(seed);
    std::uniform_real_distribution<f64> uniform(0.0, total_weight);
    for (u64 i = 0; i < N; i++) {
        // Get a Random Value
        f64 random_value = uniform(random);

        // Find the appropriate range address in weights (by binary search)
        auto range_address = std::lower_bound(cumulative_weights.begin(),
                                     cumulative_weights.end(),
                                     random_value);
        // Address -> Index: O(1)
        auto range_index = std::distance(cumulative_weights.begin(), range_address);

        values.push_back(range_index);
    }

    return values;
}


#if defined(BUILD_TESTS)
#define TEST_SCOPE zipf_generator

struct Zipf_Values {
    u64 size = 1 << 20;
    u64 range = 1 << 20;
    f64 exponent = 1.15;
    u32 seed = 22;
    u64 bucket_count = 1024;
    std::vector<u64> values;
    std::vector<u64> frequencies;

    Zipf_Values()
        : values(generate_zipf_values(size, range,
                                      exponent, seed)),
          frequencies(bucket_count, 0)
    {
        for (u64 value : values) {
            frequencies[value % bucket_count]++;
        }
    }
};

internal Zipf_Values
fixture_zipf_values() {
    static const Zipf_Values fixture; 
    return fixture;
}

test(generated_valid_values) {
    const Zipf_Values& fixture = fixture_zipf_values();

    assert(fixture.values.size() == fixture.size,
           "Generator should produce N values",
           "Actual number of values: ", fixture.values.size(),
           "Expected N: ", fixture.size);

    for (u64 value : fixture.values) {
        assert(value < fixture.range,
               "Generated value should be inside the requested range",
               "value: ", value,
               "range: ", fixture.range);
    }
}

test(first_buckets) {
    const Zipf_Values& fixture = fixture_zipf_values();

    u64 head_count = 0;
    for (u64 i = 0; i < 3; i++) {
        head_count += fixture.frequencies[i];
    }
    f32 head_fraction = (f32)head_count / fixture.size;

    assert(head_fraction > 0.24f, "first 3 buckets have large portion of requests",
           "head_fraction: ", head_fraction);
}

test(last_buckets) {
    const Zipf_Values& fixture = fixture_zipf_values();

    u64 tail_count = 0;
    for (u64 i = 25; i < fixture.bucket_count; i++) {
        tail_count += fixture.frequencies[i];
    }
    f32 tail_fraction = (f32)tail_count / fixture.size;
    assert(tail_fraction > 0.45f,
           "buckets after 25th (tail) have large portion of requests",
           "tail_fraction: ", tail_fraction);
}


Test_Case zipf_generator_tests[] = {
    test_case(first_buckets),
    test_case(last_buckets),
    test_case(generated_valid_values),
};

#undef TEST_SCOPE
#endif
