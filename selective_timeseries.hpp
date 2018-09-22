/**
 * @brief Selective time series container template

 * @file selective_timeseries.hpp
 * @author Stefan Hamminga <s@stefanhamminga.com>
 * @date 2018-09-12
 * 
 * This container template stores a fixed-size set of samples from a time series
 * dataset, always keeping the best scoring samples.
 * 
 * Some design choices:
 * 1. Access through `[]` or the forward iterator always gives elements in
 *    (reverse) insertion order.
 * 2. If the maximum amount of samples is not yet in use the iterator will only
 *    return stored samples.
 * 3. Iterators and the `[]` operator return an `element`, the value property
 *    contains the stored sample.
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
#include <array>
#include <type_traits>
#include <cstddef>

/**
 * @brief Store selected samples of a timeseries, based on a score (0 being
 * best, higher = worse) and allow efficient in-order access.
 * 
 * @tparam T_value Value type
 * @tparam S       Samples to store
 * @tparam Reverse Iteration order: false == "oldest first", true == "newest first"
 * @tparam T_time  Timestamp type 
 * @tparam T_score Score type
 */
template <typename T_value, std::size_t S, bool Reverse = false, typename T_time = std::size_t, typename T_score = float>
class selective_timeseries {
private:
    using size_t = std::size_t;

    struct element {
        T_value value;
        T_time  timestamp;
        T_score score;
    };

    std::array<element,  S> elements {};
    std::array<element*, S> index {0};

    std::size_t utilized {0};
    T_time last_timestamp_plus_one {0};

    constexpr auto worst_index() noexcept {
        T_score w = 0;
        size_t wi = 0;

        if (utilized < S) {
            if constexpr (Reverse) {
                for (size_t i = S - utilized; i < S; ++i) {
                    if (index[i]->score > w) {
                        w = index[i]->score;
                        wi = i;
                    }
                }
            } else {
                for (size_t i = 0; i < utilized; ++i) {
                    if (index[i]->score > w) {
                        w = index[i]->score;
                        wi = i;
                    }
                }
            }
        } else {
            for (size_t i = 0; i < S; ++i) {
                if (index[i]->score > w) {
                    w = index[i]->score;
                    wi = i;
                }
            }
        }
        return wi;
    }

    constexpr bool _add(const T_value& val, const T_time& timestamp, const T_score& score) noexcept {
        last_timestamp_plus_one = timestamp + 1;

        if (utilized < S) {
            elements[utilized] = { val, timestamp, score };
            if constexpr (Reverse) {
                index[S - utilized - 1] = &elements[utilized];
            } else {
                index[utilized] = &elements[utilized];
            }
            ++utilized;
            return true;
        } else {
            const auto w = worst_index();
            if (score < index[w]->score) { // store newest element in case of same score
                *index[w] = { val, timestamp, score };
                const auto temp = index[w];
                if constexpr (Reverse) {
                    std::move(index.begin(), index.begin() + w, index.begin() + 1);
                    index[0] = temp;
                } else {
                    // std::rotate generates a huge amount of extra assembly,
                    // something fishy going on there.
                    std::move(index.begin() + w + 1, index.end(), index.begin() + w);
                    index[S-1] = temp;
                }
                return true;
            }
        }
        return false;
    }

    class iterator {
    public:
        using value_type = T_value;

        constexpr iterator(std::array<element*, S>& _ptrs, const size_t _i = 0) noexcept : ptrs{_ptrs}, i{_i} {}
        constexpr iterator& operator++() noexcept { ++i; return *this; }
        constexpr bool      operator!=(const iterator& other) const noexcept { return i != other.i; }
        constexpr const auto& operator* () const noexcept { return *ptrs[i]; }
        constexpr       auto& operator* ()       noexcept { return *ptrs[i]; }
    // private:
        std::array<element*, S>& ptrs;
        size_t i;
    };

public:
    /** @brief Type of element.value */
    using value_type = T_value;

    /** @brief Count of unscored samples added. User is responsible for
               resetting after scoring. */
    std::size_t dirty { 0 };

    /**
     * @brief Return the amount of samples currently stored.
     * 
     * @return size_t Samples 
     */
    constexpr auto size() const noexcept {
        return utilized;
    }

    /**
     * @brief Add a sample to the dataset with "last time + 1" as timestamp, a
     * max score and increment the dirty counter.
     * 
     * @param  val      Sample to add
     * @return size_t   dirty count
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
     * @return size_t       Dirty count
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
     * @return size_t       Dirty count
     */
    constexpr auto add(const T_value& val, const T_time& timestamp, const T_score& score) noexcept {
        _add(val, timestamp, score);
        return dirty;
    }

    /**
     * @brief Remove element by address (pointer)
     * 
     * @param e     Element pointer to remove
     * @return bool Removed?
     */
    constexpr auto remove(const element *const e) noexcept {
        if (utilized < S) {
            for (size_t i = 0; i < utilized; ++i) {
                if (index[i] == e) {
                    if constexpr (Reverse) {
                        std::move(index.end() - utilized, index.begin() + i, index.end() - utilized + 1);
                        index[S - utilized - 1] = nullptr;
                    } else {
                        std::move(index.begin() + i + 1, index.end(), index.begin() + i);
                        index[S - 1] = nullptr;
                    }
                    --utilized;
                    return true;
                }
            }
        } else {
            for (size_t i = 0; i < S; ++i) {
                if (index[i] == e) {
                    if constexpr (Reverse) {
                        std::move(index.begin(), index.begin() + i, index.begin() + 1);
                        index[0] = nullptr;
                    } else {
                        index[i] = nullptr;
                        std::rotate(index.begin() + i, index.begin() + i + 1, index.begin() + utilized);
                    }
                    --utilized;
                    return true;
                }
            }
        }
        return false;
    }

    /** @brief shorthand for `add(const T_value& val)` */
    constexpr auto& operator+=(const T_value& val) noexcept { add(val); return this; }
    /** @brief shorthand for `remove(const element *const e)` */
    constexpr auto& operator-=(const element *const e) noexcept { remove(e); return this; }

    /**
     * @brief Find the first element with `timestamp` `time`, nullptr if not found.
     * 
     * @param time      Search time
     * @return element* Matching element or `nullptr` 
     */
    constexpr auto* find_by_exact_time(const T_time& time) noexcept {
        if constexpr (Reverse) {
            for (size_t i = S - utilized; i < S; ++i) {
                if (index[i]->timestamp == time) return index[i];
                if (index[i]->timestamp < time) break;
            }
        } else {
            for (size_t i = 0; i < utilized; ++i) {
                if (index[i]->timestamp == time) return index[i];
                if (index[i]->timestamp > time) break;
            }
        }
        return nullptr;
    }

    /**
     * @brief Find the element with `timestamp` closest to a given time. Returns
     * bogus result for an empty set!
     * 
     * @param time      Search time
     * @return element& Closest element
     */
    constexpr element& find_closest_time(const T_time& time) noexcept {
        element &res = this[0];
        T_time bestdiff = std::numeric_limits<T_time>::max();

        for (element& i : this) {
            if (time == i.timestamp) {
                return i;
            } else if (time > i.timestamp) {
                const auto diff = time - i.timestamp;
                if (diff < bestdiff) {
                    bestdiff = diff;
                    res = i;
                } else {
                    if constexpr (Reverse) {
                        break; // We are past the best time
                    }
                }
            } else if (time < i.timestamp) {
                const auto diff = i.timestamp - time;
                if (diff < bestdiff) {
                    bestdiff = diff;
                    res = i;
                } else {
                    if constexpr (!Reverse) {
                        break; // We are past the best time
                    }
                }
            }
        }
        return res;
    }

    constexpr element& worst() noexcept {
        return *index[worst_index()];
    }

    /**
     * @brief Return the, at most, N best scoring elements pointers.
     * 
     * @tparam N                        Result size
     * @return std::array<element*, N>  Array of element pointers
     */
    template <size_t N>
    constexpr auto best() noexcept {
        std::array<element*, N> res {nullptr};
        if constexpr (N >= S) {
            std::copy(index.begin(), index.end(), res.begin());
        } else {
            if constexpr (Reverse) {
                if (utilized < S) {
                    if (utilized > N) {
                        std::copy(index.end() - utilized, index.end() - utilized + N, res.begin());
                        size_t wi = 0;
                        for (size_t j = 0; j < N; ++j) {
                            if (res[j]->score > res[wi]->score) {
                                wi = j;
                            }
                        }
                        for (size_t i = (S - utilized); i < S; ++i) {
                            if (index[i]->score < res[wi]->score) {
                                std::move(res.begin() + wi + 1, res.end(), res.begin() + wi);
                                res[N-1] = index[i];

                                for (size_t j = 0; j < N; ++j) {
                                    if (res[j]->score > res[wi]->score) {
                                        wi = j;
                                    }
                                }
                            }
                        }
                    } else {
                        std::copy(index.end() - utilized, index.end(), res.begin());
                    }
                } else {
                    std::copy(index.end() - utilized, index.end() - utilized + N, res.begin());
                    size_t wi = 0;
                    for (size_t j = 0; j < N; ++j) {
                        if (res[j]->score > res[wi]->score) {
                            wi = j;
                        }
                    }
                    for (size_t i = 0; i < S; ++i) {
                        if (index[i]->score < res[wi]->score) {
                            std::move(res.begin() + wi + 1, res.end(), res.begin() + wi);
                            res[N-1] = index[i];

                            for (size_t j = 0; j < N; ++j) {
                                if (res[j]->score > res[wi]->score) {
                                    wi = j;
                                }
                            }
                        }
                    }
                }
            } else {
                if (utilized < S) {
                    if (utilized > N) {
                        std::copy(index.begin(), index.begin() + N, res.begin());
                        size_t wi = 0;
                        for (size_t j = 0; j < N; ++j) {
                            if (res[j]->score > res[wi]->score) {
                                wi = j;
                            }
                        }
                        for (size_t i = N; i < utilized; ++i) {
                            if (index[i]->score < res[wi]->score) {
                                std::move(res.begin() + wi + 1, res.end(), res.begin() + wi);
                                res[N-1] = index[i];

                                for (size_t j = 0; j < N; ++j) {
                                    if (res[j]->score > res[wi]->score) {
                                        wi = j;
                                    }
                                }
                            }
                        }
                    } else {
                        std::copy(index.begin(), index.begin() + utilized, res.begin());
                    }
                } else {
                    std::copy(index.begin(), index.begin() + N, res.begin());
                    size_t wi = 0;
                    for (size_t j = 0; j < N; ++j) {
                        if (res[j]->score > res[wi]->score) {
                            wi = j;
                        }
                    }
                    for (size_t i = N; i < S; ++i) {
                        if (index[i]->score < res[wi]->score) {
                            std::move(res.begin() + wi + 1, res.end(), res.begin() + wi);
                            res[N-1] = index[i];

                            for (size_t j = 0; j < N; ++j) {
                                if (res[j]->score > res[wi]->score) {
                                    wi = j;
                                }
                            }
                        }
                    }
                }
            }
        }
        return res;
    }

    constexpr element& operator[](const size_t n) noexcept {
        if constexpr (Reverse) {
            return *index[S - utilized + n];
        } else {
            return *index[n];
        }
    }

    constexpr iterator begin() noexcept {
        if constexpr (Reverse) {
            return { index, S - utilized };
        } else {
            return { index };
        }
    }
    constexpr iterator end() noexcept {
        if constexpr (Reverse) {
            return { index, S };
        } else {
            return { index, utilized };
        }
    }
};
