#include "own_priority_queue.h"

void SimplePriorityQueue::sort_desc()
{
    std::sort(data.begin(), data.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b)
              {
                  if (a.first != b.first)
                      return a.first > b.first; 
                  return a.second > b.second; });
}

void SimplePriorityQueue::push(const std::pair<int, int> &value)
{
    data.push_back(value);
    sort_desc();
}

void SimplePriorityQueue::pop()
{
    if (data.empty())
        throw std::runtime_error("Queue is empty");
    data.erase(data.begin());
}

const std::pair<int, int> &SimplePriorityQueue::top()
{
    if (data.empty())
        throw std::runtime_error("Queue is empty");
    return data[0];
}

const std::pair<int, int> &SimplePriorityQueue::second_top()
{
    if (data.size() < 2)
        throw std::runtime_error("Less than 2 elements in the queue");
    return data[1];
}

bool SimplePriorityQueue::x_is_in_first_k_elements(int x, int k)
{
    for (size_t i = 0; i < k; i++)
    {
        if (data[i].second == x)
        {
            return true;
        }
    }
    return false;
}

void SimplePriorityQueue::remove_first_occurence_of_x(int x)
{
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        if (it->second == x)
        {
            data.erase(it);
            break;
        }
    }
    sort_desc();
}

bool SimplePriorityQueue::empty() const
{
    return data.empty();
}

size_t SimplePriorityQueue::size() const
{
    return data.size();
}

std::string SimplePriorityQueue::as_printable()
{
    std::string result = "[";
    for (auto el : data)
    {
        result += '(';
        result += std::to_string(el.first);
        result += ':';
        result += std::to_string(el.second);
        result += ')';
        result += ',';
    }
    result += ']';
    return result;
}
