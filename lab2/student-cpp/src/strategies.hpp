#pragma once
// Стратегии выбора следующего потока.
// FirstAlive — этап 1, Replay — этап 2, RandomPick — этап 3, Dfs — этап 4 (ограничение bound — этап 5).
// enabled — номера потоков, которые могут сделать шаг, по возрастанию; last — прошлый выбранный (-1 в начале).
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "strategy.hpp"

namespace sched {

// Всегда первый поток из enabled.
struct FirstAlive : Strategy {
    int pick(const std::vector<int>& enabled, int) override {
        return enabled.front();
    }
};

// Проигрывает трассу: на i-м решении выбирает поток trace[i] ('A' -> 0, 'B' -> 1).
struct Replay : Strategy {
    explicit Replay(std::string trace) : trace_(std::move(trace)) {}
    // Сбросить позицию в трассе перед прогоном.
    void begin_run() override { idx_ = 0; }
    int pick(const std::vector<int>&, int) override { return trace_[idx_++] - 'A'; }

private:
    std::string trace_;
    size_t idx_ = 0;
};

// Случайный поток: enabled[rnd() % enabled.size()], rnd — std::mt19937(seed).
struct RandomPick : Strategy {
    explicit RandomPick(uint32_t seed) : rnd_(seed) {}
    int pick(const std::vector<int>& enabled, int) override {
        return enabled[rnd_() % enabled.size()];
    }

private:
    std::mt19937 rnd_;
};

// Stateless DFS по дереву выборов: каждый прогон с нуля, повторяет префикс прошлого и берёт следующий вариант.
// bound — максимум вытеснений (этап 5); для dfs runner передаёт INT_MAX.
struct Dfs : Strategy {
    explicit Dfs(int) {}
    // Подготовиться к новому прогону (позиция в пути, счётчик вытеснений).
    void begin_run() override { idx_ = 0; }
    int pick(const std::vector<int>& enabled, int) override {
        if (idx_ == nodes_stack.size()) nodes_stack.push_back({enabled, 0});
        auto& node = nodes_stack[idx_++];
        return node.options[node.picked];
    }
    // Выбрать следующий путь; false — дерево обойдено.
    bool end_run() override {
        while (!nodes_stack.empty()) {
            auto& node = nodes_stack.back();
            if (++node.picked < node.options.size()) return true;
            nodes_stack.pop_back();
        }
        return false;
    }

private:
    struct Node {
        std::vector<int> options;
        size_t picked;
    };
    std::vector<Node> nodes_stack;
    size_t idx_ = 0;
};

}  // namespace sched
