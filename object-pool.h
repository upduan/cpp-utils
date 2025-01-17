#include <mutex>
#include <optional>
#include <vector>

#include "Log.h"

namespace util {
    template <typename T> class ObjectPool {
    public:
        ObjectPool(int min_size, int max_size, std::function<std::pair<bool, T>()>&& creator, std::function<void(T const& obj)>&& destroy)
            : max_size_{max_size}, min_size_{min_size}, creator_{std::move(creator)}, destroy_{std::move(destroy)} {
            obj_list_.reserve(max_size);
            initialize_objects_();
        }

        virtual ~ObjectPool() {
            destroy_objects_();
        }

        void ForceReset() noexcept {
            destroy_objects_();
            initialize_objects_();
        }

        std::optional<T> GetObject() noexcept {
            std::optional<T> result;
            std::lock_guard<std::mutex> lock(mutex_);
            auto r = find_object_([](auto it) { return !it->busy; });
            if (r.first) {
                it->busy = true;
                result = it->object;
                log_trace << "find object";
            } else {
                if (count_ < max_size_) {
                    auto const& [success, obj] = creator_();
                    if (success) {
                        result = obj;
                        obj_list_.emplace_back(ObjectWrapper(obj, true));
                        log_info << "create and pool new object";
                        count_++;
                    } else {
                        log_error << "create new object failure";
                    }
                } else {
                    log_info << "reached max object count, please wait object idle";
                }
            }
            return result;
        }

        void RecycleObject(T const& obj) noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            auto r = find_object_([&obj](auto it) { return it->object == obj; });
            if (r.first) {
                it->busy = false;
                log_trace << "RecycleObject";
            } else {
                log_info << "RecycleObject not found";
            }
        }

        void RemoveObject(T const& obj) noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            auto r = find_object_([&obj](auto it) { return it->object == obj; });
            if (r.first) {
                destroy_(obj);
                obj_list_.erase(r.second);
                count_--;
                log_trace << "RemoveObject";
            } else {
                log_info << "RemoveObject not found";
            }
        }

    protected:
        void initialize_object_() noexcept {
            for (int i = 0; i < min_size_; ++i) {
                auto const& [success, obj] = creator_();
                if (success) {
                    obj_list_.emplace_back(ObjectWrapper(obj));
                }
            }
            count_ = min_size_;
        }

        void destroy_objects_() noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto it = obj_list_.begin(); it != obj_list_.end(); ++it) {
                destroy_(obj);
            }
            obj_list_.clear();
            count_ = 0;
        }

        std::pair<bool, std::vector<ObjectWrapper>::iterator> find_object_(std::function<bool(std::vector<ObjectWrapper>::iterator item)>&& predicate) noexcept {
            auto end = obj_list_.end();
            for (auto it = obj_list_.begin(); it != end; ++it) {
                if (predicate(it)) {
                    return {true, it};
                }
            }
            return {false, end};
        }

    private:
        struct ObjectWrapper {
            ObjectWrapper(const T& obj, bool abusy = false) : object{obj}, busy{abusy} {}
            ObjectWrapper(T&& obj, bool abusy = false) : object{std::move(obj)}, busy{abusy} {}
            T object;
            mutable bool busy;
        };
        int max_size_;
        int min_size_;
        int count_;
        std::mutex mutex_;
        std::vector<ObjectWrapper> obj_list_;
        std::function<std::pair<bool, T>()> creator_;
        std::function<void(T const& obj)> destroy_;
    };
} // namespace util
