#pragma once

#include <chrono>
#include <cstdint>

enum class TimingSection {
    // k2 covers movement logic and k3 covers pheromone marking work.
    k2,
    k3
};

struct TimingProfileTotals {
    // Accumulate fine-grained per-iteration work-time inside ant kernels.
    std::uint64_t k2_ns{0};
    std::uint64_t k3_ns{0};
};

// Read a monotonic timestamp in nanoseconds for local timing sections.
inline std::uint64_t profile_now_ns();

class TimingProfile
{
public:
    // Clear accumulated work-time and disable profiling until the runner enables it.
    void reset()
    {
        m_totals = TimingProfileTotals{};
        m_enabled = false;
    }

    // Enable or disable fine-grained work-time accumulation for the current iteration.
    void set_enabled(bool enabled) { m_enabled = enabled; }
    bool enabled() const { return m_enabled; }

    // Return the start timestamp for one fine-grained timed section.
    std::uint64_t start(TimingSection /*section*/) const { return profile_now_ns(); }
    void stop(TimingSection section, std::uint64_t start_ns) { add(section, profile_now_ns() - start_ns); }

    // Add a measured duration into the matching fine-grained timing bucket.
    void add(TimingSection section, std::uint64_t duration_ns)
    {
        if (!m_enabled) {
            return;
        }
        if (section == TimingSection::k2) {
            m_totals.k2_ns += duration_ns;
        } else {
            m_totals.k3_ns += duration_ns;
        }
    }

    const TimingProfileTotals& totals() const { return m_totals; }

private:
    TimingProfileTotals m_totals{};
    bool m_enabled{false};
};

inline std::uint64_t profile_now_ns()
{
    // Use steady_clock so benchmark timings are not affected by wall-clock jumps.
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now().time_since_epoch()).count());
}
