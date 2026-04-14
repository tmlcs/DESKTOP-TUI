#ifndef TUI_CORE_SIGNAL_HPP
#define TUI_CORE_SIGNAL_HPP

#include <functional>
#include <vector>
#include <cstdint>

namespace tui {

/// Lightweight signal/slot for UI events (simpler than EventBus, for direct connections)
template<typename... Args>
class Signal {
public:
    using SlotId = uint64_t;
    using Callback = std::function<void(Args...)>;

    SlotId connect(Callback cb) {
        SlotId id = next_id_++;
        slots_.push_back({id, std::move(cb)});
        return id;
    }

    void disconnect(SlotId id) {
        slots_.erase(
            std::remove_if(slots_.begin(), slots_.end(),
                [id](const auto& s) { return s.id == id; }),
            slots_.end()
        );
    }

    void emit(Args... args) const {
        // FIX C7: snapshot to prevent iterator invalidation if callback disconnects
        auto snapshot = slots_;
        for (const auto& [id, cb] : snapshot) {
            cb(args...);
        }
    }

    void clear() { slots_.clear(); }
    bool empty() const { return slots_.empty(); }

private:
    struct Slot {
        SlotId id;
        Callback callback;
    };

    SlotId next_id_ = 1;
    std::vector<Slot> slots_;
};

} // namespace tui

#endif // TUI_CORE_SIGNAL_HPP
