#pragma once

#include <cstdint>
#include <deque>

template <class T> // must have an int64_t field called ts
class History {
    using Iterator = typename std::deque<T>::iterator;

    std::deque<T> m_items;

public:
    bool Empty() const { return m_items.empty(); }

    bool Push(T val) {
        if (Empty() || m_items.back().ts < val.ts) {
            m_items.push_back(std::move(val));
            return true;
        }

        return false;
    }

    const T *Back() const { return Empty() ? nullptr : &m_items.back(); }

    const T *FetchBack() { 
        Cleanup(m_items.end());
        return Back();
    }

    const T *FetchFirstAfter(int64_t ts) {
        auto it = BinSearch(ts);
        if (it == m_items.end() || it->ts < ts)
            return nullptr;

        Cleanup(it);
        return &(*it);
    }

    const T *FetchLastBefore(int64_t ts) {
        auto it = BinSearch(ts);
        if (it == m_items.end() || m_items.front().ts > ts)
            return nullptr;

        Cleanup(it);
        return it->ts <= ts ? &(*it) : &(*(it-1));
    }

    std::pair<const T *, const T *> FetchTwoClosest(int64_t ts) {
        std::pair<const T *, const T *> res;
        auto it = BinSearch(ts);

        if (it == m_items.end())
            res.first = res.second = nullptr;
        else if (it->ts == ts)
            res.first = res.second = &(*it);
        else if (it == m_items.begin()) {
            res.first = nullptr;
            res.second = &(*it);
        } else if (it->ts > m_items.back().ts) {
            res.first = &(*it);
            res.second = nullptr;
        } else {
            res.first = &(*(it-1));
            res.second = &(*it);
        }

        Cleanup(it);
        return res;
    }

private:
    void Cleanup(Iterator second_after) {
        if (second_after != m_items.begin()) 
            m_items.erase(m_items.begin(), second_after-1);
    }

    Iterator BinSearch(int64_t ts) {
        if (m_items.empty())
            return m_items.end();

        auto lo = m_items.begin();
        auto hi = m_items.end() - 1;

        if (ts >= hi->ts)
            return hi;
        else if (ts <= lo->ts)
            return lo;

        size_t dist;
        while ((dist = std::distance(lo, hi)) > 1) {
            auto mid = lo + dist/2;
            if (mid->ts > ts)
                hi = mid;
            else if (mid->ts < ts)
                lo = mid;
            else
                return mid;
        }

        return hi;
    }
};
