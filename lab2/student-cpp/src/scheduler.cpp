// Этап 1–2: реализация Scheduler, point() и await().
// Сейчас все методы бросают исключение «TODO»: заполните их по заданию этапов 1 и 2.
// Деструктор должен дождаться (join) всех запущенных потоков, в том числе после deadlock.
#include "scheduler.hpp"
#include "sched_mutex.hpp"

#include <algorithm>
#include <cassert>
#include <semaphore>
#include <thread>
#include <utility>

namespace sched {

struct Scheduler::Worker {
    Worker(int id, std::function<void()> fn) : thread_id(id), body(std::move(fn)), semaphore(0) {}
    int thread_id;
    std::function<void()> body;
    std::thread thread;
    std::binary_semaphore semaphore;
    bool done = false;
};

struct RunAborted {};

Scheduler::Scheduler() = default;

struct TlsState {
    Scheduler* scheduler = nullptr;
    int thread_id = 0;
    bool pause_before_next_shared_access = false;
};
static thread_local TlsState tls_state;

Scheduler::~Scheduler() {
    for (auto& worker : threads_) if (worker->thread.joinable()) worker->thread.join();
}

void Scheduler::spawn(std::function<void()> body) {
    threads_.push_back(std::make_unique<Worker>(static_cast<int>(threads_.size()), std::move(body)));
    waiting_mutexes_.push_back(nullptr);
}

void Scheduler::run_worker(Scheduler* scheduler, int thread_id) {
    auto& worker = *scheduler->threads_[thread_id];
    tls_state = {scheduler, thread_id, true};
    try { worker.body(); }
    catch (const RunAborted&) { worker.done = true; return; }
    worker.done = true;
    scheduler->semaphore_.release();
}

void Scheduler::run(Strategy& strategy) {
    // Make threads sleep in their semaphores
    for (size_t i = 0; i < threads_.size(); ++i) {
        auto& worker = *threads_[i];
        worker.thread = std::thread(&Scheduler::run_worker, this, worker.thread_id);
        semaphore_.acquire();
    }

    int last = -1;
    while (true) {
        std::vector<int> enabled;
        for (size_t i = 0; i < threads_.size(); ++i)
            if (!threads_[i]->done &&
                (!waiting_mutexes_[i] || waiting_mutexes_[i]->owner < 0))
                enabled.push_back(static_cast<int>(i));
        if (enabled.empty()) {
            bool all_done = true;
            for (const auto& worker : threads_) all_done &= worker->done;
            if (all_done) break;
            deadlock = make_held_string();
            abort_workers();
            return;
        }

        const int picked = strategy.pick(enabled, last);
        assert(std::find(enabled.begin(), enabled.end(), picked) != enabled.end()); // valid thread_id
        trace_ += 'A' + picked;
        last = picked;
        auto& worker = *threads_[static_cast<size_t>(picked)];
        worker.semaphore.release();
        semaphore_.acquire();
    }
    for (auto& worker : threads_) if (worker->thread.joinable()) worker->thread.join();
}

std::string Scheduler::trace_string() const { return trace_; }

void Scheduler::pause() {
    auto& tls = tls_state;
    if (!tls.scheduler || !tls.pause_before_next_shared_access) return;
    tls.pause_before_next_shared_access = false;
    tls.scheduler->yield(tls.thread_id);
}

int Scheduler::current_thread_id() { return tls_state.thread_id; }

void Scheduler::yield(int thread_id) {
    semaphore_.release();
    threads_[thread_id]->semaphore.acquire();
    if (aborted_) throw RunAborted{};
}

void Scheduler::register_mutex(sched_mutex* mutex) {
    mutexes_.push_back(mutex);
}

std::string Scheduler::make_held_string() const {
    std::string held = "held={";
    bool first = true;
    for (const auto* mutex : mutexes_) if (mutex->owner >= 0) {
        if (!first) held += ", ";
        first = false;
        held += mutex->name + ":" + char('A' + mutex->owner);
    }
    return held + "}";
}

void Scheduler::abort_workers() {
    aborted_ = true;
    for (auto& worker : threads_) if (!worker->done) worker->semaphore.release();
    for (auto& worker : threads_) if (worker->thread.joinable()) worker->thread.join();
}

void point() {
    if (tls_state.scheduler && tls_state.scheduler->logging)
        tls_state.scheduler->log.push_back(std::string(1, 'A' + tls_state.thread_id) + "  window");
    tls_state.pause_before_next_shared_access = true;
}

void await(std::function<bool()> cond) {
    assert(cond());
    point();
}

void log_operation(const std::string& operation) {
    if (!tls_state.scheduler) return;
    tls_state.pause_before_next_shared_access = true;
    if (tls_state.scheduler->logging)
        tls_state.scheduler->log.push_back(std::string(1, 'A' + tls_state.thread_id) + "  " + operation);
}

}  // namespace sched
