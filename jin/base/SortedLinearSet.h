#pragma once

#include <vector>
#include <algorithm>

/* 
*/
template <typename ElementType>
class SortedLinearSet
{
public:
    int count(const ElementType& val)
    {
        return std::binary_search(a.cbegin(), a.cend(), val) ? 1 : 0;
    }

    void insert(const ElementType& val)
    {
        a.emplace_back(val);
        std::inplace_merge(begin(a), end(a) - 1, end(a));
    }

private:
    std::vector<ElementType> a;
};