#pragma once

template<typename... Args>
class Signal {
public:
    using SlotType = std::function<void(Args...)>;
    using SlotID = int;

    SlotID connect(SlotType slot) {
        SlotID id = nextID++;
        slots[id] = std::move(slot);
        return id;
    }

    void disconnect(SlotID id) {
        slots.erase(id);
        // ---
        if (auto it = slots.find(id); it != slots.end) {
            slots.erase(it);
        }
    }

    void emit(Args... args) const {
        for (const auto& slot : slots) {
            slot.second(args...);
        }
    }

private:
    std::unordered_map<SlotID, SlotType> slots;
    SlotID nextID = 0;
};
