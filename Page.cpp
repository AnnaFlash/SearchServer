//#include "Page.h"
//template <typename Iterator>
//IteratorRange<Iterator>::IteratorRange(Iterator begin, Iterator end)
//        : begin_(begin)
//        , end_(end)
//    {}
//template <typename Iterator>
//    void IteratorRange<Iterator>::begin() const {
//        return begin_;
//    }
//template <typename Iterator>
//    void IteratorRange<Iterator>::end() const {
//        return end_;
//    }
//template <typename Iterator>
//    void IteratorRange<Iterator>::size() const {
//        return distance(begin_, end_);
//    }


//template <typename Iterator>
//Paginator<Iterator>::Paginator(Iterator begin, Iterator end, size_t size) {
//        while (distance(begin, end) > size) {
//            auto page = IteratorRange(begin, advance(begin, size));
//            pages_.push_back(page);
//            begin = advance(begin, size);
//        }
//        pages_.push_back(IteratorRange(begin, end));
//
//    }
//template <typename Iterator>
//void Paginator<Iterator>::begin() const {
//        return pages_.begin();
//    }
//template <typename Iterator>
//    void Paginator<Iterator>::end() const {
//        return pages_.end();
//    }
//template <typename Iterator>
//    void Paginator<Iterator>::size() const {
//        return pages_.size();
//    }
