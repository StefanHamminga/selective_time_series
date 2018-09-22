# C++ Selective Time Series container

C++ container template to manage and store the best scoring subset of a time series dataset, allowing chronological and reverse iteration.

## Usage & example

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
#include <selective_timeseries/selective_timeseries.hpp>
int main() {
    ...
    selective_timeseries<float, 1000> s;
    ...
    s.add(1.23f);
    ...
    for (auto& e : s) {
        do_something1(e.value); // 1.23f, etc
        do_something2(e.timestamp);
        do_something3(e.score);
    }
}
```

For the full API consult the header file or generate the complete documentation.

## License

This work is dual-licensed under GPL 2, and LGPL 3.0 or any later version.
You can choose between one of them if you use this work.

`SPDX-License-Identifier: GPL-2.0-or-later OR LGPL-3.0-or-later`
