#include <vector>
#include <map>
#include <string>
#include <mutex>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct MapPart {
        std::map<Key, Value> map_;
        std::mutex mutex_;
    };
    
    struct Access {
        Access(const Key& key, MapPart& map_part)
            : guard(map_part.mutex_)
            , ref_to_value(map_part.map_[key]) {
        }
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
        :  concurrent_maps_(bucket_count) {}

    Access operator[](const Key& key) {
        auto& map_part = concurrent_maps_[static_cast<uint64_t>(key) % concurrent_maps_.size()];
        return Access(key, map_part);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;

        for (auto& [map_, mutex_] : concurrent_maps_) {
            std::lock_guard guard(mutex_);
            result.insert(map_.begin(), map_.end());
        }

        return result;
    }

    void erase(const Key& key) {
        auto& map_part = concurrent_maps_[static_cast<uint64_t>(key) % concurrent_maps_.size()];
        map_part.map_.erase(key);
    }

private:
    std::vector<MapPart> concurrent_maps_;
};
