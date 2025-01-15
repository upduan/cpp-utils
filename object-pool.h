#include <mutex>
#include <optional>
#include <vector>

#include "Log.h"

namespace util {
    // template <typename T, typename = void> struct has_cleanup : std::false_type {};

    // template <typename T> struct has_cleanup<T, std::void_t<decltype(std::declval<T>().cleanup())>> : std::true_type {};

    // 对象池
    // 可以结合单例模式使用
    template <typename T> class ObjectPool {
    private:
        template <typename U> struct ObjectWrapper {
            ObjectWrapper(const U& obj, bool abusy = false) : object{obj}, busy{abusy} {}
            ObjectWrapper(U&& obj, bool abusy = false) : object{std::move(obj)}, busy{abusy} {}
            U object;
            mutable bool busy;
        };
        int max_size_;
        int min_size_;
        int count_;
        std::mutex mutex_;
        // std::vector<std::shared_ptr<T>> obj_list_;
        std::vector<ObjectWrapper<T>> obj_list_;
        std::function<std::pair<bool, T>()> creator_;
        std::function<void(T const& obj)> destroy_;
        // std::function<void(T* obj_ptr)> obj_destroy_func_;
        // std::atomic_bool is_destructed_;

    public:
        ObjectPool(int min_size, int max_size, std::function<std::pair<bool, T>()>&& creator, std::function<void(T const& obj)>&& destroy)
            : max_size_{max_size}, min_size_{min_size}, creator_{std::move(creator)}, destroy_{std::move(destroy)} {
            // static_assert(has_cleanup<T>::value, "T must have a cleanup() function");
            obj_list_.reserve(max_size);
            // obj_destroy_func_ = [&](T* obj_ptr) {
            //     obj_ptr->cleanup();
            //     if (is_destructed_) {
            //         delete obj_ptr;
            //     } else {
            //         // 加入回收池
            //         std::lock_guard<std::mutex> lock(mutex_);
            //         obj_list_.push_back(std::shared_ptr<T>(obj_ptr, obj_destroy_func_));
            //     }
            // };
            initialize_create_object();
        }

        virtual ~ObjectPool() {
            std::lock_guard<std::mutex> lock(mutex_);
            // is_destructed_ = true;
            for (auto& obj : obj_list_) {
                destroy_(obj.object);
            }
        }

        void ForceReset() noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            obj_list_.clear();
            initialize_create_object();
        }

        // std::shared_ptr<T> GetObject() {
        std::optional<T> GetObject() noexcept {
            // std::shared_ptr<T> result;
            std::optional<T> result;
            std::lock_guard<std::mutex> lock(mutex_);
            bool is_find = false;
            for (auto const& obj_wrapper : obj_list_) {
                if (!obj_wrapper.busy) {
                    result = obj_wrapper.object;
                    obj_wrapper.busy = true;
                    log_debug << "find object";
                    is_find = true;
                    break;
                }
            }
            if (!is_find) { // not found
                if (count_ < max_size_) {
                    auto const& [success, obj] = creator_();
                    if (success) {
                        result = obj;
                        obj_list_.emplace_back(ObjectWrapper<T>(obj, true));
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
            for (auto it = obj_list_.begin(); it != obj_list_.end(); ++it) {
                if (it->object == obj) {
                    it->busy = false;
                    log_trace << "RecycleObject";
                    return;
                }
            }
            log_info << "RecycleObject not found";
        }

        void RemoveObject(T const& obj) noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto it = obj_list_.begin(); it != obj_list_.end(); ++it) {
                if (it->object == obj) {
                    obj_list_.erase(it);
                    count_--;
                    log_trace << "RemoveObject";
                    return;
                }
            }
            log_info << "RemoveObject not found";
        }

    private:
        void initialize_create_object() noexcept {
            for (int i = 0; i < min_size_; ++i) {
                // obj_list_.emplace_back(new T(), obj_destroy_func_);
                auto const& [success, obj] = creator_();
                if (success) {
                    obj_list_.emplace_back(ObjectWrapper<T>(obj));
                }
            }
            count_ = min_size_;
        }
    };
} // namespace util
