# C++ Selective Time Series container

C++ container template to manage and store the $N$ best scoring subset of a time series dataset, allowing chronological and reverse iteration.

For larger set sizes this library performs considerably better than a simple library.

Some design choices:
1. Access through `[]` or the forward iterator always gives elements in
   (reverse) insertion order.
2. If the maximal amount of samples is not yet in use the iterator will only
   return stored samples.
3. Iterators and the `[]` operator return a `std::tuple<value&, timestamp&, score&>`.
4. A timestamp can be given. If omitted $previously\_highest\_timestamp + 1$ will be used.
5. $0$ is considered the *best* score, higher = worse. Scores are assumed positive.
6. A score can be provided on adding a sample, if omitted 0 will be used.
7. A `dirty` counter is incremented each time a sample without score is
   added. Use for partial re-scoring:
      `for (size_t i = ts.size() - ts.dirty; i < ts.size(); ++i) rescore(ts[i]);`
   The user should reset it to 0.

## Usage & example

The library is header-only, making installation (copying the header to the system location) pretty trivial.

Installation through CMake:

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
# Run tests
# make check
# Generate documentation
# make doc
sudo make install
```

Simple example:

```c++
#include <selective_time_series/selective_time_series.hpp>
int main() {
    ...
    selective_time_series<float, 1000> ts;
    ...
    ts.add(1.23f);
    ...
    for (auto& [value, timestamp, score] : ts) {
        do_something1(value); // 1.23f, etc
        do_something2(timestamp);
        do_something3(score);
    }
}
```

For the full API consult the header file or generate the complete documentation.

## License

This work is dual-licensed under GPL 2, and LGPL 3.0 or any later version.
You can choose between one of them if you use this work.

`SPDX-License-Identifier: GPL-2.0-or-later OR LGPL-3.0-or-later`
