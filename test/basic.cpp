#include "../selective_timeseries.hpp"

#include <iostream>
#include <iomanip>
#include <random>
#include <cstddef>

int main() {
    std::default_random_engine e { 1u }; // Will result in the same 'random' generation each compile
    std::uniform_real_distribution<> rnd {0.0f, 1.0f};
    std::cout << std::setprecision(3) ;

    selective_timeseries<float, 1000, false> s;

    for (std::size_t i = 0; i < 30'000; ++i) {
        const auto score = rnd(e);
        s.add(rnd(e), i, score);
        std::cout << "Added " << i
                  << ", score " << score
                  << ", worst " << s.worst().score
                  << ", size " << s.size()
                  << '\n';

        for (const auto& n : s) {
            std::cout << n.timestamp << " ";
        }
        std::cout << '\n';
        for (const auto& n : s) {
            std::cout << n.score << " ";
        }
        std::cout << '\n';


    }
    for (auto* n : s.best<11>()) {
        if (n == nullptr) break; // Should only happen when N > .size()
        std::cout << n->score << " ";
    }
    std::cout << '\n';
}