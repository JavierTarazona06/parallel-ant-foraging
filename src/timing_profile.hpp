#pragma once

#include <chrono>
#include <cstdint>

enum class TimingSection {
    k2,
    k3
};

struct TimingProfileTotals {
    std::uint64_t k2_ns{0};
    std::uint64_t k3_ns{0};
};

inline std::uint64_t profile_now_ns();

class TimingProfile
{
public:
    void reset()
    {
        m_totals = TimingProfileTotals{};
        m_enabled = false;
    }

    void set_enabled(bool enabled) { m_enabled = enabled; }
    bool enabled() const { return m_enabled; }

    std::uint64_t start(TimingSection /*section*/) const { return profile_now_ns(); }
    void stop(TimingSection section, std::uint64_t start_ns) { add(section, profile_now_ns() - start_ns); }

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
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now().time_since_epoch()).count());
}
