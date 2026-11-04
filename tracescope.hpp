#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace tracescope {
using Clock = std::chrono::steady_clock;
struct Event {
    char name[48]{};
    std::int64_t start_ns = 0, duration_ns = 0;
    unsigned thread = 0;
};
// Bounded multi-producer recorder. Its mutex favors simplicity over lock-free speed.
class Recorder {
    const Clock::time_point epoch_ = Clock::now();
    mutable std::mutex mutex_;
    std::vector<Event> ring_;
    std::size_t next_ = 0, count_ = 0;
    std::uint64_t total_ = 0;
    static std::string escape(const char* s) {
        std::string out;
        constexpr char hex[] = "0123456789abcdef";
        for (const unsigned char* p = reinterpret_cast<const unsigned char*>(s); *p; ++p) {
            if (*p == '"' || *p == '\\') { out += '\\'; out += static_cast<char>(*p); }
            else if (*p < 0x20) { out += "\\u00"; out += hex[*p >> 4]; out += hex[*p & 15]; }
            else out += static_cast<char>(*p);
        }
        return out;
    }
public:
    explicit Recorder(std::size_t capacity) : ring_(capacity) {
        if (!capacity) throw std::invalid_argument("capacity must be positive");
    }
    std::int64_t timestamp() const noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - epoch_).count();
    }
    void record(const char* name, std::int64_t start, std::int64_t duration, unsigned thread) {
        Event e;
        if (name) for (std::size_t i=0; i<sizeof(e.name)-1 && name[i]; ++i) e.name[i]=name[i];
        e.start_ns = start;
        e.duration_ns = std::max<std::int64_t>(0, duration);
        e.thread = thread;
        std::lock_guard<std::mutex> lock(mutex_);
        ring_[next_] = e;
        next_ = (next_ + 1) % ring_.size();
        count_ = std::min(count_ + 1, ring_.size());
        ++total_;
    }
    std::vector<Event> snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<Event> events;
        events.reserve(count_);
        const auto first = (next_ + ring_.size() - count_) % ring_.size();
        for (std::size_t i = 0; i < count_; ++i) events.push_back(ring_[(first + i) % ring_.size()]);
        return events;
    }
    std::uint64_t total() const { std::lock_guard<std::mutex> lock(mutex_); return total_; }
    std::size_t capacity() const { return ring_.size(); }
    std::size_t event_storage_bytes() const { return ring_.size() * sizeof(Event); }
    void export_json(const std::string& path) const {
        // Consistent snapshot; exporting is intended after worker threads join.
        const auto events = snapshot();
        std::ofstream out(path);
        if (!out) throw std::runtime_error("cannot write trace");
        out << "{\"displayTimeUnit\":\"ms\",\"traceEvents\":[";
        bool comma = false;
        for (const auto& e : events) {
            if (comma) out << ',';
            comma = true;
            out << "{\"name\":\"" << escape(e.name) << "\",\"cat\":\"scope\",\"ph\":\"X\",\"pid\":1,\"tid\":"
                << e.thread << ",\"ts\":" << (e.start_ns / 1000.0) << ",\"dur\":" << (e.duration_ns / 1000.0) << '}';
        }
        out << "],\"metadata\":{\"capacity\":" << ring_.size()
            << ",\"totalEvents\":" << total() << ",\"retainedEvents\":" << events.size()
            << ",\"overwrittenEvents\":" << (total() - events.size()) << "}}\n";
        if (!out) throw std::runtime_error("trace write failed");
    }
};
class Scope {
    Recorder& recorder_;
    char name_[48]{};
    unsigned thread_;
    std::int64_t start_;
public:
    Scope(Recorder& r, const char* name, unsigned thread) : recorder_(r), thread_(thread), start_(r.timestamp()) {
        if (name) for (std::size_t i=0; i<sizeof(name_)-1 && name[i]; ++i) name_[i]=name[i];
    }
    ~Scope() { recorder_.record(name_, start_, recorder_.timestamp() - start_, thread_); }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
};
}
