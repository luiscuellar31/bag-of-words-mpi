#pragma once
#include <chrono>

inline double now_sec() {
    using clock = std::chrono::steady_clock;
    auto t = clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::duration<double>>(t).count();
}

