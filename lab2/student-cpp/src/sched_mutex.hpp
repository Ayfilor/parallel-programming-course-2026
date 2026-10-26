#pragma once
// Этап 2: лок, о котором знает планировщик. Поток, ждущий занятый лок, не попадает в enabled;
// обычный std::mutex здесь нельзя — поток уснёт с ходом в руках и всё зависнет.
// Лог: "lock <имя>", "unlock <имя>". Планировщик должен знать все локи, чтобы печатать held={a:A, b:B}.
#include <string>

#include "scheduler.hpp"

namespace sched {

class sched_mutex {
public:
    // Создаётся в setup() тестовой программы; зарегистрируйте лок в планировщике s.
    sched_mutex(Scheduler& s, std::string name)
        : scheduler_(s), name(std::move(name)) { scheduler_.register_mutex(this); }
    sched_mutex(const sched_mutex&) = delete;

    // Пауза; если лок занят — ждать, не участвуя в выборе; затем стать владельцем.
    void lock() {
        const int id = Scheduler::current_thread_id();
        set_waiting_mutex(id);
        Scheduler::pause();
        wake_waiter(id);
        while (owner >= 0) {
            set_waiting_mutex(id);
            scheduler_.yield(id);
            wake_waiter(id);
        }
        owner = id;
        log_operation("lock " + name);
    }
    // Пауза; освободить лок, ждавшие потоки снова могут быть выбраны.
    void unlock() {
        if (scheduler_.aborted_) return;
        Scheduler::pause();
        owner = -1;
        log_operation("unlock " + name);
        wake_mutex_waiters();
    }

    const std::string name;
    int owner = -1;

private:
    void set_waiting_mutex(int id) { scheduler_.waiting_mutexes_[id] = this; }
    void wake_waiter(int id) { scheduler_.waiting_mutexes_[id] = nullptr; }
    void wake_mutex_waiters() {
        for (auto& waiting : scheduler_.waiting_mutexes_)
            if (waiting == this) waiting = nullptr;
    }

    Scheduler& scheduler_;
};

// RAII: lock сейчас, unlock на закрывающей скобке. Выдан готовым.
class sched_guard {
public:
    explicit sched_guard(sched_mutex& m) : m_(m) { m_.lock(); }
    ~sched_guard() noexcept(false) { m_.unlock(); }
    sched_guard(const sched_guard&) = delete;

private:
    sched_mutex& m_;
};

}  // namespace sched
