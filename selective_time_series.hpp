/**
 * @brief Selective time series container template

 * @file selective_time_series.hpp
 * @author Stefan Hamminga <s@stefanhamminga.com>
 * @date 2018-09-12
 * 
 * This container template stores a fixed-size set of samples from a time series
 * dataset, always keeping the best scoring samples.
 * 
 * Some design choices:
 * 1. Access through `[]` or the forward iterator always gives elements in
 *    (reverse) insertion order.
 * 2. If the maximal amount of samples is not yet in use the iterator will only
 *    return stored samples.
 * 3. Iterators and the `[]` operator return a `std::tuple<value&, timestamp&, score&>`. 
 * 4. A timestamp can be given. If omitted 'highest timestamp + 1' will be used.
 * 5. 0 is considered the _best_ score, higher = worse. Scores are assumed positive.
 * 6. A score can be provided on adding a sample, if omitted 0 will be used.
 * 7. A `dirty` counter is incremented each time a sample without score is
 *    added. Use for partial rescoring:
 *      `for (size_t i = �.size() - �.dirty; i < �.size(); ++i) rescore(�[i]);`
 *    The user is responsible for resetting it to 0.
 * 
 * Notes:
 * 1. Telling GCC by hand which branches to take (likely, etc) gains a few
 *    percent over not doing so. PGO without any indicators doubles that gain.
 */

#pragma once

#include <algorithm>
#include <tuple>
#include <array>
#include <type_traits>
#include <cstdint>
#include <cstddef>

/**
 * @brief Store selected samples of a time_series, based on a score (0 being
 * best, higher = worse) and allow efficient in-order access.
 * 
 * @tparam T_value Value type
 * @tparam S       Samples to store
 * @tparam Reverse Iteration order: false == "oldest first", true == "newest first"
 * @tparam T_time  Timestamp type 
 * @tparam T_score Score type
 */
template <typename T_value, std::size_t S, bool Reverse = false, typename T_time = std::size_t, typename T_score = float>
class selective_time_series {
private:
    enum {
        VAL = 0,
        TIM = 1,
        SCO = 2
    };
    // using size_t = std::size_t;
    using index_t = std::conditional_t<(S < 256),
                                       uint8_t,
                                       std::conditional_t<(S < 65536),
                                                          uint16_t,
                                                          std::conditional_t<(S < 4294967296),
                                                                             uint32_t, uint64_t>>>;

    std::array<T_value, S> values;
    std::array<T_time,  S> timestamps;
    std::array<T_score, S> scores;
    std::array<index_t, S> offsets;

    index_t utilized {0};
    T_time last_timestamp_plus_one {0};

    constexpr std::tuple<index_t, T_score> worst_index() noexcept {
        const auto r = std::max_element(scores.begin(), scores.end());
        return { std::distance(scores.begin(), r), *r };
    }

    constexpr index_t find_offset_index(index_t in) {
        for (index_t i = 0; i < S; ++i) {
            if (offsets[i] == in) return i;
        }
        return S;
    }

    constexpr bool _add(const T_value& val, const T_time& timestamp, const T_score& score) noexcept {
        last_timestamp_plus_one = timestamp + 1;

        if (utilized < S) {
            values[utilized] = val;
            timestamps[utilized] = timestamp;
            scores[utilized] = score;

            ++utilized;
            return true;
        } else {
            const auto [wi, ws] = worst_index();
            if (score <= ws) { // store newest element in case of same score
                values[wi] = val;
                timestamps[wi] = timestamp;
                scores[wi] = score;

                const auto oi = find_offset_index(wi);
                if constexpr (Reverse) {
                    std::move(offsets.begin(), offsets.begin() + oi, offsets.begin() + 1);
                    offsets[0] = wi;
                } else {
                    // std::rotate generates a huge amount of extra assembly,
                    // something fishy going on there.
                    std::move(offsets.begin() + oi + 1, offsets.end(), offsets.begin() + oi);
                    offsets[S-1] = wi;
                }
                return true;
            }
        }
        return false;
    }

    class iterator {
    public:
        using value_type = T_value;
        constexpr iterator(selective_time_series& ts, const index_t _i = 0) noexcept : series{ts}, i{_i} {}
        constexpr iterator& operator++()       noexcept { ++i; return *this; }
        constexpr bool      operator!=(const iterator& other) const noexcept { return i != other.i; }
        constexpr auto      operator* () const noexcept { return std::forward_as_tuple(series.values[series.offsets[i]], series.timestamps[series.offsets[i]], series.scores[series.offsets[i]]); }
        constexpr auto      operator* ()       noexcept { return std::forward_as_tuple(series.values[series.offsets[i]], series.timestamps[series.offsets[i]], series.scores[series.offsets[i]]); }
    private:
        selective_time_series& series;
        index_t i;
    };

public:
    /** @brief Type of element.value */
    using value_type = T_value;

    constexpr selective_time_series() {
        for (index_t i = 0; i < S; ++i) {
            if constexpr (Reverse) {
                offsets[i] = (S-1) - i;
            } else {
                offsets[i] = i;
            }
        }
    }

    /** @brief Count of unscored samples added. User is responsible for
               resetting after scoring. */
    index_t dirty { 0 };

    /**
     * @brief Return the amount of samples currently stored.
     * 
     * @return index_t Samples 
     */
    constexpr auto size() const noexcept {
        return utilized;
    }

    /**
     * @brief Add a sample to the dataset with "last time + 1" as timestamp, a
     * max score and increment the dirty counter.
     * 
     * @param  val      Sample to add
     * @return index_t  dirty count
     */
    constexpr auto add(const T_value& val) noexcept {
        dirty += _add(val, last_timestamp_plus_one++, 0);
        return dirty;
    }
    /**
     * @brief Add a sample to the dataset with `timestamp` as timestamp, a max
     * score and increment the dirty counter.
     * 
     * @param  val          Sample to add
     * @param  timestamp    Timestamp for sample
     * @return index_t      Dirty count
     */
    constexpr auto add(const T_value& val, const T_time& timestamp) noexcept {
        dirty += _add(val, timestamp, 0);
        return dirty;
    }
    /**
     * @brief Add a scored sample to the dataset, dirty counter is not increased.
     * 
     * @param  val          Sample to add
     * @param  timestamp    Timestamp for sample
     * @param  score        Score for sample
     * @return index_t      Dirty count
     */
    constexpr auto add(const T_value& val, const T_time& timestamp, const T_score& score) noexcept {
        _add(val, timestamp, score);
        return dirty;
    }

    constexpr auto insertion_offset(const T_time& timestamp) const noexcept {
        index_t i = 0;
        if constexpr (Reverse) {
            for (i = S - utilized; i < S; ++i) {
                if (timestamp > timestamps[offsets[i]]) return i;
            } // Data too old, insert at back
        } else {
            for (; i < utilized; ++i) {
                if (timestamp < timestamps[offsets[i]]) return i;
            } // Data too new, insert at back
        }
        return i;
    }

    constexpr bool has(const std::tuple<const T_value&, const T_time&, const T_score&>&& elem) const noexcept {
        return  std::find(timestamps.begin(), timestamps.end(), std::get<TIM>(elem)) != timestamps.end() &&
                std::find(scores.begin(), scores.end(), std::get<SCO>(elem)) != scores.end() &&
                std::find(values.begin(), values.end(), std::get<VAL>(elem)) != values.end();
    }

    /**
     * @brief Like `add(...)`, but instead of assuming the timestamp is always
     * newest, inserts at the proper location. More expensive.
     * 
     * @param  val          Sample to add
     * @param  timestamp    Timestamp for sample
     * @param  score        Score for sample
     */
    constexpr bool insert_one(const std::tuple<const T_value&, const T_time&, const T_score&>&& elem) noexcept {
        if (std::get<TIM>(elem) + 1 > last_timestamp_plus_one) {
            last_timestamp_plus_one = std::get<TIM>(elem) + 1;
        }

        if (utilized < S) {
            values[utilized] = std::get<VAL>(elem);
            timestamps[utilized] = std::get<TIM>(elem);
            scores[utilized] = std::get<SCO>(elem);

            const auto io = insertion_offset(std::get<TIM>(elem));
            
            if constexpr (Reverse) {
                auto b = offsets.begin() + S - utilized;
                std::move(b, b + io, b - 1);
            } else {
                std::move(offsets.begin() + io, offsets.begin() + utilized, offsets.begin() + io + 1);
            }
            ++utilized;
            return true;

        } else {
            const auto [wi, ws] = worst_index();

            if (std::get<SCO>(elem) > ws) {
                return false;
            }

            values[wi] = std::get<VAL>(elem);
            timestamps[wi] = std::get<TIM>(elem);
            scores[wi] = std::get<SCO>(elem);

            const auto wo = find_offset_index(wi);
            const auto io = insertion_offset(std::get<TIM>(elem));

            if (io < wo) {
                std::move(offsets.begin() + io, offsets.begin() + wo, offsets.begin() + io + 1);
                offsets[io] = wi;
            } else if (wo < io) {

                std::move(offsets.begin() + wo + 1, offsets.begin() + io, offsets.begin() + wo);
                offsets[io-1] = wi;
            }
            return true;
        }
    }

    template <std::size_t N, typename T, typename... Ts>
    struct n_th_type {
        using type = std::conditional_t<(N == 0), T, typename n_th_type<N-1, Ts...>::type>;
    };

    template <typename... Ts>
    constexpr void insert_multiple(const Ts& ...sample) {
        //TODO: merge the 'worst' search operations
        (insert_one(sample), ...);
    }

    constexpr decltype(auto) insert(const T_value& val, const T_time& timestamp, const T_score& score) {
        return insert_one(std::forward_as_tuple(val, timestamp, score));
    }

    template <typename T, typename U, typename V, std::size_t N, bool B>
    constexpr void merge(selective_time_series<T,N,B,U,V>& other) noexcept {
        //TODO: merge the 'worst' search operations
        for (const auto& e : other) {
            if (!has(e)) {
                insert_one(e);
            }
        }
    }

    /** @brief shorthand for `add(const T_value& val)` */
    constexpr auto& operator+=(const T_value& val) noexcept { add(val); return this; }

    constexpr auto worst() noexcept {
        const auto [ wi, ws ] = worst_index();
        return std::forward_as_tuple(values[wi], timestamps[wi], scores[wi]);
    }

    /**
     * @brief Return the, at most, min(N,S) best scoring elements pointers.
     * 
     * @tparam N                        Result size
     * @return std::array<element*, N>  Array of element pointers
     */
    template <index_t N, typename... Ts>
    constexpr std::array<std::tuple<T_value&, T_time&, T_score&>, N> best(Ts ...tups) noexcept {
        static_assert(N <= S, "Can't select more 'best' elements than S");
        if constexpr (sizeof...(Ts) < N) {
            return best<N, Ts..., std::tuple<T_value&, T_time&, T_score&>>(tups..., std::forward_as_tuple(values[sizeof...(Ts)], timestamps[sizeof...(Ts)], scores[sizeof...(Ts)]));
        } else {
            std::array<std::tuple<T_value&, T_time&, T_score&>, N> res { tups... };

            index_t wi = 0;
            T_score ws = std::get<SCO>(res[0]);

            for (index_t i = 1; i < N; ++i) {
                if (ws < std::get<SCO>(res[i])) {
                    wi = i;
                    ws = std::get<SCO>(res[i]);
                }
            }
            for (index_t i = N; i < utilized; ++i) {
                if (ws > scores[i]) {
                    res[wi] = { values[i], timestamps[i], scores[i] };
                    for (index_t j = 0; j < N; ++j) {
                        if (ws < std::get<SCO>(res[j])) {
                            wi = j;
                            ws = std::get<SCO>(res[j]);
                        }
                    }
                }
            }
            // Looks like the STL has an issue sorting tuples of references.
            // Need to follow-up.
            // if constexpr (Reverse) {
            //     std::sort(res.begin(), res.end(), [](const auto& a, const auto& b) { return std::get<TIM>(a) - std::get<TIM>(b); });
            // } else {
                // std::sort(res.begin(), res.end(), [](const auto& a, const auto& b) { return std::get<TIM>(b) < std::get<TIM>(a); });
            // }
            for (index_t i = 0; i < N; ++i) {
                if constexpr (Reverse) {
                    for (index_t j = i; j > 0 && (std::get<TIM>(res[j - 1]) < std::get<TIM>(res[j])); --j) {
                        res[j].swap(res[j-1]);
                    }
                } else {
                    for (index_t j = i; j > 0 && (std::get<TIM>(res[j - 1]) > std::get<TIM>(res[j])); --j) {
                        res[j].swap(res[j-1]);
                    }
                }
            }
            return res;
        }
    }

    constexpr auto operator[](const index_t n) noexcept {
        if constexpr (Reverse) {
            const auto o = offsets[S - utilized + n];
            return std::forward_as_tuple(values[o], timestamps[o], scores[o]);
        } else {
            const auto o = offsets[n];
            return std::forward_as_tuple(values[o], timestamps[o], scores[o]);
        }
    }

    constexpr iterator begin() noexcept {
        return { *this, Reverse ? static_cast<index_t>(S - utilized) : static_cast<index_t>(0) };
    }
    constexpr iterator end() noexcept {
        return { *this, Reverse ? static_cast<index_t>(S) : utilized };
    }
};
