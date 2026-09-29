// ========================================================================
// =                             Interfaces                               =
// ========================================================================

#define _NUM_BUCKETS 256
#define _BUCKET_SPAN_MS 4

#define _U64_UPPER_LIMIT (u64)(0 - 1)

struct Snapshot {
    std::array<u64, _NUM_BUCKETS> buckets;
    u64 count;
    u64 sum;
    u64 min;
    u64 max;
    u64 p50;
    u64 p99;
};

class Metrics_Collector {
public:
    virtual ~Metrics_Collector() = default;
    virtual void record(u64 request_delay_ms) = 0;
    virtual Snapshot get_snapshot() = 0;
};


struct Thread_State {
    std::array<std::atomic<u64>, _NUM_BUCKETS> buckets = {0};
    std::atomic<u64> count{0};
    std::atomic<u64> sum{0};
    std::atomic<u64> min{_U64_UPPER_LIMIT};
    std::atomic<u64> max{0};
};


// ========================================================================
// =                          Implementations                             =
// ========================================================================

class Single_Threaded : public Metrics_Collector {
public:
    void record(u64 request_delay_ms) override;
    Snapshot get_snapshot() override;
private:
    std::array<u64, _NUM_BUCKETS> buckets = {0};
    u64 count = 0;
    u64 sum = 0;
    u64 min = _U64_UPPER_LIMIT;
    u64 max = 0;
};



// =========================   Global Lock   ==========================

class Global_Lock_Collector : public Single_Threaded {
public: 
    void record(u64 request_delay_ms) override;
    Snapshot get_snapshot() override;
private:
    std::mutex mutex;
};


// =========================   Stub Lock   ==========================


class Stub_Lock_Collector : public Single_Threaded {
public: 
    void record(u64 request_delay_ms) override;
    Snapshot get_snapshot() override;
private:
    std::mutex mutex;
};


// =========================   Stripped Lock   ==========================


#define _NUM_BUCKET_GROUPS (_NUM_BUCKETS / 16)

class Striped_Lock_Collector : public Metrics_Collector {
public: 
    void record(u64 request_delay_ms) override;
    Snapshot get_snapshot() override;
private:
    std::array<std::mutex, _NUM_BUCKET_GROUPS> bucket_groups_mutexes;

    std::array<u64, _NUM_BUCKETS> buckets = {0};
    std::atomic<u64> count{0};
    std::atomic<u64> sum{0};
    std::atomic<u64> min{_U64_UPPER_LIMIT};
    std::atomic<u64> max{0};
};


// =========================   Thread Local   ==========================

internal inline u64
next_collector_id() {
    static std::atomic<u64> counter{1};
    return counter.fetch_add(1);
}

class Thread_Local_Collector : public Metrics_Collector {
public: 
    void record(u64 request_delay_ms) override;
    Snapshot get_snapshot() override;
private:
    Thread_State* get_my_state();

    const u64 id = next_collector_id();
    std::mutex list_lock;
    std::vector<std::unique_ptr<Thread_State>> thread_states_list;
};


// =========================   Double Buffer   ==========================

struct Thread_Buffer {
    std::array<u64, _NUM_BUCKETS> buckets = {0};
    u64 count = 0;
    u64 sum = 0;
    u64 min = _U64_UPPER_LIMIT;
    u64 max = 0;
};
void metrics_collector__clear_buffer(Thread_Buffer* buffer) {
    buffer->buckets.fill(0);
    buffer->count = 0;
    buffer->sum = 0;
    buffer->min = _U64_UPPER_LIMIT;
    buffer->max = 0;
}

enum Selected_Buffer { Neither = -1, First = 0, Second = 1 };
struct alignas(64) Thread_Buffers {
    std::atomic<Selected_Buffer> inside{Selected_Buffer::Neither};
    Thread_Buffer buf[2];
};

// The main idea:
//   For each Writer we store 2 buffers;
//   Reader traverses the Writers and swaps the buffer in each Writer,
//   and copies the last valid state into the global buffer (global state)
//
// The states for Workers are stored
class Double_Buffer_Collector : public Metrics_Collector {
public: 
    void record(u64 request_delay_ms) override;
    Snapshot get_snapshot() override;
private:
    Thread_Buffers* get_my_buffers();

    const u64 id = next_collector_id();
    std::atomic<Selected_Buffer> active_buffer = {Selected_Buffer::First};
    Thread_Buffer global_buffer;
    std::mutex list_lock;
    std::vector<std::unique_ptr<Thread_Buffers>> thread_states_list;
};
