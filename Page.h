#include "headers.h"
template <typename It>
class IteratorRange {
public:
    IteratorRange(It begin, It end)
        : begin_(begin)
        , end_(end)
    {}

    auto begin() const {
        return begin_;
    }

    auto end() const {
        return end_;
    }

    auto size() const {
        return distance(begin_, end_);
    }
private:
    It begin_;
    It end_;
};

template <typename It>
class Paginator {
public:
    Paginator(It begin, It end, size_t size) {
        while (distance(begin, end) > size) {
            auto page = IteratorRange(begin, begin + size);
            pages_.push_back(page);
            begin = begin + size;
        }
        pages_.push_back(IteratorRange(begin, end));

    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    auto size() const {
        return pages_.size();
    }
private:
    std::vector<IteratorRange<It>> pages_;
};
