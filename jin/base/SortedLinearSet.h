#pragma once

#include <vector>
#include <algorithm>

/* 
*/
template <typename ElementType>
class SortedLinearSet
{
public:
    int count(const ElementType& val) const
    {
        return std::binary_search(a.cbegin(), a.cend(), val) ? 1 : 0;
    }

    void insert(const ElementType& val)
    {
        if (count(val) == 1)
        {
            return;
        }
        a.emplace_back(val);
        std::inplace_merge(begin(a), end(a) - 1, end(a));
    }

    int size() const
    {
        return a.size();
    }

private:
    std::vector<ElementType> a;
};