#include "../selective_time_series.hpp"

#include <iostream>
#include <iomanip>
#include <random>
#include <cstddef>

int main() {
    constexpr size_t S = 14;
    constexpr size_t I = 47;

    std::default_random_engine e { 1u }; // Will result in the same 'random' generation each compile
    std::uniform_real_distribution<> rnd {0.0f, 1.0f};
    std::cout << std::setprecision(3) ;

    selective_time_series<double, S, false> ts1;
    selective_time_series<double, S, true> ts2;

    std::cout << "Doing " << I << " random additions to two " << S << "-sized time series:\n";
    for (std::size_t i = 0; i < 47; ++i) {
        const auto val = rnd(e);
        const auto score = rnd(e);
        ts1.add(val, i, score);
        ts2.add(val, i, score);
        std::cout << "Added " << i
                  << ", score " << score
                  << ", worst " << std::get<2>(ts1.worst())
                  << ", size " << (size_t)ts1.size() << "/" << (size_t)ts2.size()
                  << '\n';

        for (const auto& [v, t, s] : ts1) {
            std::cout << t << " ";
        }
        std::cout << '\n';
        for (const auto& [v, t, s] : ts1) {
            std::cout << s << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\nInserting an item in each...\n";

    ts1.insert(0, 3, 0.011);
    ts2.insert(0, 3, 0.011);


    std::cout << "\nMerging both time series...\n";
    ts2.merge(ts1);

    std::cout << "\nTimestamps, in order:\n";
    for (const auto& [v, t, s] : ts1) {
        std::cout << t << " ";
    }
    std::cout << '\n';
    for (const auto& [v, t, s] : ts2) {
        std::cout << t << " ";
    }

    std::cout << "\n\nScores, in order:\n";
    for (const auto& [v, t, s] : ts1) {
        std::cout << s << " ";
    }
    std::cout << '\n';
    for (const auto& [v, t, s] : ts2) {
        std::cout << s << " ";
    }
    std::cout << '\n';

    // for (const auto& [v,t,s] : ts1.best<11>()) {
    //     // if (n == nullptr) break; // Should only happen when N > .size()
    //     std::cout << s << " ";
    // }
    // std::cout << '\n';
}