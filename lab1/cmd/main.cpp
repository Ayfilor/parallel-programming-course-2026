#include "../imports.h"


#define _WARM_UP_SECONDS std::chrono::seconds(5)
#define _MEASURE_SECONDS std::chrono::seconds(5)
#define _NUM_MEASURES 5
#define _MEASURE_ROUND_COEF 1e-6

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

internal f64
run_benchmark(Metrics_Collector* collector,
              const std::vector<u64>& values,
              u32 num_threads,
              std::chrono::seconds bench_duration_sec) 
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

    // Run the bench!
    auto start_time_sec = std::chrono::steady_clock::now();
    {
        start_latch.count_down();
        std::this_thread::sleep_for(bench_duration_sec);
        stop_flag.store(true);
    }
    auto end_time_sec = std::chrono::steady_clock::now();

    // maybe join before stopping the timer? Some operations still evals
    for (u64 worker_index = 0; worker_index < num_threads; worker_index++) {
        threads_list[worker_index].join();
    }

    u64 total_ops = 0;
    for (u32 worker_index = 0; worker_index < num_threads; worker_index++) {
        total_ops += ops[worker_index];
    }

    f64 duration_sec = 
        std::chrono::duration<f64>(end_time_sec - start_time_sec).count(); 

    return total_ops / duration_sec;
}

internal f64
measure_point(Metrics_Collector* collector,
              const std::vector<u64>& values,
              u32 num_threads,
              u32 num_measures)
{
    // warm up
    run_benchmark(collector, values, num_threads, _WARM_UP_SECONDS);    

    // Measure bench N times
    std::vector<f64> results; results.reserve(num_measures);
    for (u32 _ = 0; _ < num_measures; _++) results.push_back(
        run_benchmark(collector, values, num_threads, _MEASURE_SECONDS)
    );

    // Just to prevent compiler to delete code. Reports including warm_up
    std::cout << "Count: " << collector->get_snapshot().count << std::endl;

    // Calculate median and normalize it to Millions
    std::sort(results.begin(), results.end());
    return results[results.size() / 2] * _MEASURE_ROUND_COEF;
}


int main() {
    auto values = generate_zipf_values(1<<20, 1024, 1.15, 42);
    auto collector = Double_Buffer_Collector();

    u64 num_threads_measures[5] = {1, 2, 4, 8, 16};
    for (auto num_threads : num_threads_measures) {
        auto measure = measure_point(&collector, values, num_threads, _NUM_MEASURES);
        std::cout << "Measure Point(" << num_threads << "): " 
                  << measure << std::endl << std::endl;
    }

    return 0;
}
