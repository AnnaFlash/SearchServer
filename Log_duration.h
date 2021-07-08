#pragma once

#include <chrono>
#include <iostream>
#include <string_view>

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
//#define LOG_DURATION_STREAM(a, out) LogDuration UNIQUE_VAR_NAME_PROFILE(a, out)
class LogDuration {
public:
    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string_view& id, std::ostream& out = std::cerr)
        : id_(id), out_(out) {
    }

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;
        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        out_ << "Operation time: " << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    const std::string_view id_;
    const Clock::time_point start_time_ = Clock::now();
    std::ostream& out_ = std::cerr;
};