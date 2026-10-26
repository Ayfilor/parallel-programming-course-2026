#pragma once
// Этап 1: планировщик, который ведёт настоящие потоки строго по одному (передача хода через семафоры).
// Этап 2: обнаружение deadlock в главном цикле и корректное завершение застрявших потоков.
// Внутреннее устройство (поля, семафоры, вспомогательные функции для shared и sched_mutex) придумываете сами.
#include <functional>
#include <memory>
#include <semaphore>
#include <string>
#include <vector>

#include "strategy.hpp"

namespace sched {

class sched_mutex;

class Scheduler {
public:
    Scheduler();
    ~Scheduler();
    // Зарегистрировать поток. Потоки получают буквы A, B, C… в порядке вызова spawn.
    void spawn(std::function<void()> body);
    // Запустить потоки и вести их по одному: на каждой паузе спросить strategy.pick(enabled, last).
    void run(Strategy& strategy);
    // Трасса прогона: буквы потоков, выбранных во всех решениях подряд, например "AAAABB".
    std::string trace_string() const;
    static void pause();
    void yield(int thread_id);
    static int current_thread_id();
    void register_mutex(sched_mutex* mutex);

    // Если true, каждая операция с общими данными добавляет строку в log. Выставляет runner.
    bool logging = false;
    // Лог прогона: строки "<поток>  <операция>", например "A  lock mu", "A  balance -> 100".
    std::vector<std::string> log;
    // Пустая строка, если deadlock не было; иначе "held={a:A, b:B}" (этап 2).
    std::string deadlock;
    bool aborted_ = false;
    std::vector<sched_mutex*> waiting_mutexes_;

private:
    struct Worker;
    static void run_worker(Scheduler* scheduler, int thread_id);
    std::string make_held_string() const;
    void abort_workers();
    std::vector<std::unique_ptr<Worker>> threads_;
    std::vector<sched_mutex*> mutexes_;
    std::string trace_;
    std::binary_semaphore semaphore_{0};
};

// window() из тестовых программ: пауза перед следующей операцией с общими данными обязательна.
void point();
// Ждать, пока cond не станет true. До этого поток не попадает в enabled.
void await(std::function<bool()> cond);
// Internal operation logger used by Shared.
void log_operation(const std::string& operation);

}  // namespace sched
