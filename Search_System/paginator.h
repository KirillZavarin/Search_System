#pragma once

#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    explicit IteratorRange(const Iterator& begin, const Iterator& end)
        : begin_(begin), end_(end)
    {}

    auto begin() const {
        return begin_;
    }

    auto end() const {
        return end_;
    }

    auto size() const {
        return std::distance(begin_, end_);
    }

private:
    const Iterator begin_;
    const Iterator end_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, IteratorRange<Iterator> iters) {
    for (auto iter = iters.begin(); iter < iters.end(); iter++) {
        out << *iter;
    }
    return out;
}

template <typename Iterator>
class Paginator {
public:

    Paginator(Iterator begin, Iterator end, size_t page_size) {
        for (auto i = begin; std::distance(i, end) > 0; std::advance(i, page_size)) {
            if (std::distance(i, end) >= page_size) {
                pages_.push_back(IteratorRange<Iterator>{i, std::next(i, page_size)});
            }
            else {
                pages_.push_back(IteratorRange<Iterator>{i, end});
                break;
            }
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
    // òåëî êëàññà
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(c.begin(), c.end(), page_size);
}