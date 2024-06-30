//
// Created by Declan Walsh on 20/06/2024.
//

#ifndef JUMPTABLE_H
#define JUMPTABLE_H

#include <map>
#include "Value.h"
#include "ScriptUtils.h"

template<typename VALUE_T, typename ADDR_T>
class JumpTable {

public:
    DataType Type;

    JumpTable() = default;

    bool Add(VALUE_T value, ADDR_T address) {
        const auto add = m_Items.emplace(value, address);
        return add.second;
    }

    int Size() {
        return m_Items.size() * sizeof (ADDR_T);
    }

    int Count() {
        return m_Items.size();
    }

    bool Find(VALUE_T value, ADDR_T &outAddr) {
        const auto it = m_Items.find(value);
        if (it == m_Items.end()) {
            return false;
        }

        outAddr = it->second;
        return true;
    }

    VALUE_T LowestValue() {
        if (m_Items.empty())
            return {};

        return m_Items.begin()->first;
    }

    VALUE_T HighestValue() {
        if (m_Items.empty())
            return {};

        return std::prev(m_Items.end())->first;
    }

private:
    std::map<VALUE_T, ADDR_T> m_Items;
};

#endif //JUMPTABLE_H
