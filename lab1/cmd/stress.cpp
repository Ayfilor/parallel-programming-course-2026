#include "../imports.h"

#define STRESS_TEST__NUM_WORKERS 4
#define STRESS_TEST__NUM_ITERATIONS 10000


internal void
worker(Metrics_Collector* collector,
       const std::vector<u64>& values,
       u64 worker_index,
       std::latch* start_latch,
       std::atomic<bool>* stop_flag,
       std::vector<u64>* ops)
{
    u64 local_count = 0;
    u64 index = (worker_index * 1000) % values.size(); //out-of-bound harness

    start_latch->wait();
    while (!stop_flag->load()) {
        collector->record(values[index]);
        local_count++;
        index++;
        if (index == values.size()) index = 0;
    }

    (*ops)[worker_index] = local_count; 
}


internal void
stress_test__run(Metrics_Collector* collector,
                 const std::vector<u64>& values,
                 u64 num_threads)
{
    std::latch start_latch(1);
    std::atomic<bool> stop_flag(false);
    std::vector<u64> ops(num_threads, 0);
    std::vector<std::thread> threads_list;

    for (u64 worker_index = 0; worker_index < num_threads; worker_index++) {
        threads_list.emplace_back(std::thread(worker,
                                              collector, std::cref(values),
                                              worker_index,
                                              &start_latch, &stop_flag, &ops));
    }

    // Run the stress test!
        start_latch.count_down();

        u64 sum_buckets__greater_than__count_metric = 0;
        u64 sum_buckets__lower_than__count_metric = 0;
        for (u64 i = 0; i < STRESS_TEST__NUM_ITERATIONS; i++) {
            auto snapshot = collector->get_snapshot();

            u64 sum_buckets = 0; for (u64 b : snapshot.buckets) sum_buckets += b;

            if (sum_buckets > snapshot.count) {
                sum_buckets__greater_than__count_metric++; 
            } else if (sum_buckets < snapshot.count) {
                sum_buckets__lower_than__count_metric++;
            }
            // maybe sleep for a little?
        }

        stop_flag.store(true);

        for (u64 worker_index = 0; worker_index < num_threads; worker_index++) {
            threads_list[worker_index].join();
        }

    u64 total_ops = 0;
    for (u32 worker_index = 0; worker_index < num_threads; worker_index++) {
        total_ops += ops[worker_index];
    }



    u64 num_inconsistant_snapshots = sum_buckets__greater_than__count_metric 
                                   + sum_buckets__lower_than__count_metric;
    f64 fraction_inconsistant_snapshots = ((f64)num_inconsistant_snapshots 
                                        / STRESS_TEST__NUM_ITERATIONS) * 100.0;

    //print
    std::cout << "Fraction: " << fraction_inconsistant_snapshots << std::endl; 
    std::cout << "Greater: " << sum_buckets__greater_than__count_metric <<std::endl; 
    std::cout << "Lower: " << sum_buckets__lower_than__count_metric << std::endl; 
    std::cout << "Final Count: " << collector->get_snapshot().count << std::endl; 
    std::cout << "Workers Requests: " << total_ops << std::endl; 
}

int main() {
    auto values = generate_zipf_values(1<<20, 1<<20, 1.15, 42);
    auto collector = Double_Buffer_Collector();

    stress_test__run(&collector, values, STRESS_TEST__NUM_WORKERS);
}
