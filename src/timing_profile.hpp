#pragma once

#include <chrono>
#include <cstdint>

struct TimingProfileTotals {
    std::uint64_t k2_ns{0};
    std::uint64_t k3_ns{0};
};

extern TimingProfileTotals g_timing_profile_totals;
extern bool g_timing_profile_enabled;

inline std::uint64_t profile_now_ns()
{
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now().time_since_epoch()).count());
}

inline void profile_add_k2(std::uint64_t duration_ns)
{
    if (g_timing_profile_enabled) {
        g_timing_profile_totals.k2_ns += duration_ns;
    }
}

inline void profile_add_k3(std::uint64_t duration_ns)
{
    if (g_timing_profile_enabled) {
        g_timing_profile_totals.k3_ns += duration_ns;
    }
}
