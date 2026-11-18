// DispatchQueue.h  --  UNIT IV: generic class template + functor (function object)
#ifndef DISPATCHQUEUE_H
#define DISPATCHQUEUE_H

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "Inventory.h"

namespace ReliefOps::Logistics {

// A generic queue of anything (cargo, missions, ...). Order is decided by a functor
// that the caller passes to sortBy().  Internally it uses std::vector + std::sort.
template <typename T>
class PriorityDispatchQueue {
    std::vector<T> items_;

public:
    void push(const T& item) { items_.push_back(item); }
    bool empty() const { return items_.empty(); }
    std::size_t size() const { return items_.size(); }

    template <typename Compare>
    void sortBy(Compare cmp) {  // Compare can be a functor, a lambda or a function pointer
        std::sort(items_.begin(), items_.end(), cmp);
    }

    const T& top() const {
        if (items_.empty()) throw std::out_of_range("PriorityDispatchQueue is empty");
        return items_.front();
    }

    T pop() {  // removes and returns the first element (call sortBy first)
        if (items_.empty()) throw std::out_of_range("PriorityDispatchQueue is empty");
        T first = items_.front();
        items_.erase(items_.begin());
        return first;
    }

    template <typename Func>
    void forEach(Func f) const {
        std::for_each(items_.begin(), items_.end(), f);
    }

    typename std::vector<T>::const_iterator begin() const { return items_.begin(); }
    typename std::vector<T>::const_iterator end() const { return items_.end(); }
};

// Functor: "who goes first?"  Highest priority first; on a tie, Medical beats everything else;
// remaining ties are broken alphabetically so the result is always the same.
struct MorePressingNeed {
    bool operator()(const Inventory::ReliefCargo& a, const Inventory::ReliefCargo& b) const {
        if (a.getPriority() != b.getPriority()) return a.getPriority() > b.getPriority();
        bool aMedical = (a.getCategory() == "Medical");
        bool bMedical = (b.getCategory() == "Medical");
        if (aMedical != bMedical) return aMedical;
        return a.getName() < b.getName();
    }
};

}  // namespace ReliefOps::Logistics

#endif
