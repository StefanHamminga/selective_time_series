#include "../selective_time_series.hpp"

#include <iostream>
#include <iomanip>
#include <random>
#include <cstddef>

int main() {
    std::default_random_engine e { 1u }; // Will result in the same 'random' generation each compile
    std::uniform_real_distribution<> rnd {0.0f, 1.0f};
    std::cout << std::setprecision(3) ;

    selective_time_series<std::array<double, 8>, 100'000, false> ts;

    for (std::size_t i = 0; i < 200'000; ++i) {
        const auto score = rnd(e);
        ts.add({ rnd(e), rnd(e), rnd(e), rnd(e), rnd(e), rnd(e), rnd(e), rnd(e) }, i, score);
        // std::cout << "Added " << i
        //           << ", score " << score
        //           << ", worst " << std::get<2>(ts.worst())
        //           << ", size " << ts.size()
        //           << '\n';

        // for (const auto& [v, t, s] : ts) {
        //     std::cout << t << " ";
        // }
        // std::cout << '\n';
        // for (const auto& [v, t, s] : ts) {
        //     std::cout << s << " ";
        // }
        // std::cout << '\n';


    }

    ts.insert({ rnd(e), rnd(e), rnd(e), rnd(e), rnd(e), rnd(e), rnd(e), rnd(e) }, 99, 0);

    for (const auto& [v,t,s] : ts.best<11>()) {
        // if (n == nullptr) break; // Should only happen when N > .size()
        std::cout << s << " ";
    }
    std::cout << '\n';
}