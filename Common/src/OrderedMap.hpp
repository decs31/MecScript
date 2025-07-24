//
// Created by Declan Walsh on 12/06/2024.
//

#ifndef ORDEREDMAP_HPP
#define ORDEREDMAP_HPP

#include <unordered_map>
#include <vector>

template <typename TKey, typename TValue> class OrderedMap
{
  public:
    typedef bool (*Predicate)(TValue &value);

    OrderedMap()  = default;
    ~OrderedMap() = default;

    size_t Size()
    {
        return m_Items.size();
    }

    bool Add(const TKey &key, const TValue &value)
    {
        size_t index           = m_Items.size();
        const auto [it, added] = m_ItemMap.insert(key, index);
        if (!added) {
            return false;
        }

        m_Items.emplace_back(value);
        return true;
    }

    bool Remove(const TKey &key)
    {
        auto it = m_ItemMap.find(key);

        if (it == m_ItemMap.end()) {
            return false;
        }

        size_t removedIndex = it->second;
        m_Items.erase(m_Items.begin() + removedIndex);

        // Reduce all the indexes above the removed item by one.
        for (auto &item : m_ItemMap) {
            if (item.second > removedIndex) {
                --item.second;
            }
        }

        return true;
    }

    bool Contains(const TValue &value) const
    {
        return m_ItemMap.contains(value);
    }

    TValue &Find(TKey &key) const
    {
        auto it = m_ItemMap.find(key);
        if (it != m_ItemMap.end()) {
            return m_Items[it->second];
        }
    }

    TValue &Search(Predicate predicate) const
    {
        if (predicate == nullptr) {
            return nullptr;
        }

        for (auto &item : m_Items) {
            if (predicate(item)) {
                return &item;
            }
        }

        return nullptr;
    }

    TValue &At(size_t index) const
    {
        return m_Items.at(index);
    }

    TValue &operator[](size_t idx)
    {
        return m_Items[idx];
    }

  private:
    std::unordered_map<TKey, size_t> m_ItemMap;
    std::vector<TValue> m_Items;
};

#endif // ORDEREDMAP_HPP
