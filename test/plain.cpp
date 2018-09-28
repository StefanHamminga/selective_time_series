#include <iostream>
#include <iomanip>
#include <random>
#include <cstddef>

int main() {
    std::default_random_engine e { 1u }; // Will result in the same 'random' generation each compile
    std::uniform_real_distribution<> rnd {0.0f, 1.0f};
    std::cout << std::setprecision(3) ;

    // selective_timeseries<std::array<double, 8>, 1'000, false> ts;

    struct sample {
        std::array<double, 8> values {0};
        std::size_t timestamp {0};
        float score = 0; 
    };

    std::array<sample, 100'000> ts;

    for (std::size_t i = 0; i < 200'000; ++i) {
        const auto score = rnd(e);
        if (i < ts.size()) {
            ts[i] = {{ rnd(e), rnd(e), rnd(e), rnd(e), rnd(e), rnd(e), rnd(e), rnd(e) }, i, score};
        } else {
            std::size_t wi = 0;
            float ws = ts[0].score;
            for (std::size_t j = 1; j < ts.size(); ++j) {
                if (ts[j].score > ws) {
                    ws = ts[j].score;
                    wi = j;
                }
            }
            if (score <= ws) {
                std::move(ts.begin() + wi + 1, ts.end(), ts.begin() + wi);
                ts[ts.size() - 1] = {{ rnd(e), rnd(e), rnd(e), rnd(e), rnd(e), rnd(e), rnd(e), rnd(e) }, i, score};
            }
        }

        // std::size_t wi = 0;
        float ws = ts[0].score;
        for (std::size_t j = 1; j < ts.size(); ++j) {
            if (ts[j].score > ws) {
                ws = ts[j].score;
                // wi = j;
            }
        }

        // std::cout << "Added " << i
        //           << ", score " << score
        //           << ", worst " << ws
        //           << ", size " << ts.size()
        //           << '\n';

        // for (int k = 0, kl = (std::min(ts.size(), i)); k < kl; ++k) {
        //     std::cout << ts[k].timestamp << " ";
        // }
        // std::cout << '\n';

        // for (int k = 0, kl = (std::min(ts.size(), i)); k < kl; ++k) {
        //     std::cout << ts[k].score << " ";
        // }
        // std::cout << '\n';
    }
    std::array<sample*, 11> best;
    for (size_t i = 0; i < 11; ++i) {
        best[i] = &ts[i];
    }
    for (size_t i = 11; i < ts.size(); ++i) {
        for (size_t j = 0; j < best.size(); ++j) {
            if (best[j]->score > ts[i].score) {
                std::move(best.begin() + j + 1, best.end(), best.begin() + j);
                best[best.size() - 1] = &ts[i];
                break;
            }
        }
    }
    for (const auto *const b : best) {
        // if (n == nullptr) break; // Should only happen when N > .size()
        std::cout << b->score << " ";
    }
    std::cout << '\n';
}