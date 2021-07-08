#pragma once
#include "search_server.h"
using namespace std;
vector<vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const vector<string>& queries) {
    vector<vector<Document>> result(queries.size());
    transform(execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](auto querie)
        {return search_server.FindTopDocuments(querie); });
    return result;
}
class Wrapper_Process_Queries {
    using Type = Document;
    using Wrapper_Matrix_Type = vector<vector<Type>>;
    using Wrapper_Type = vector<Type>;
private: 
    Wrapper_Matrix_Type wrapper_result_;
public:
    Wrapper_Process_Queries(Wrapper_Matrix_Type wrapper_result) 
        : wrapper_result_(wrapper_result) {}
    
    class Wrapper_Iterator {
        using M_Iterator = typename Wrapper_Matrix_Type::const_iterator;
        using Iterator = typename Wrapper_Type::const_iterator;
    private:
        M_Iterator begin_;
        M_Iterator end_;
        Iterator iterator_;
    public:
        Wrapper_Iterator(M_Iterator begin, M_Iterator end) : begin_(begin), end_(end) {
            if (begin_ != end_) { iterator_ = begin_->begin(); }
        }
        Wrapper_Iterator& operator++() {
            if (begin_ == end_) {
                return *this;
            }
            if (iterator_ != begin_->end()) {
                iterator_++;
            }
            if (iterator_ == begin_->end()) {
                begin_++;
                if (begin_ != end_) {
                    iterator_ = begin_->begin();
                }
            }
            return *this;
        }
        Type operator*() {
            return *iterator_;
        }
        bool operator==(Wrapper_Iterator& other) {
            return (other.end_ == end_ && begin_ == end_);
        }
        bool operator!=(Wrapper_Iterator& other) {
            return !(*this == other);
        }
    };
    Wrapper_Iterator begin() {
        return Wrapper_Iterator(wrapper_result_.begin(), wrapper_result_.end());
    }
    Wrapper_Iterator end() {
        return Wrapper_Iterator(wrapper_result_.begin(), wrapper_result_.end());
    }
};
Wrapper_Process_Queries ProcessQueriesJoined(const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    return ProcessQueries(search_server, queries);
}