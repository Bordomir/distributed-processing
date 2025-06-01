#ifndef SIMPLE_PRIORITY_QUEUE_H
#define SIMPLE_PRIORITY_QUEUE_H

#include <vector>
#include <utility>
#include <algorithm>
#include <stdexcept>

class SimplePriorityQueue
{
private:
    std::vector<std::pair<int, int>> data;

    void sort_desc();

public:
    void push(const std::pair<int, int> &value);
    void pop();
    const std::pair<int, int> &top();
    const std::pair<int, int> &second_top();
    bool x_is_in_first_k_elements(int x, int k);
    bool empty() const;
    size_t size() const;
    std::string as_printable();
};

#endif
