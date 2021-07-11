#include "Search_Server.h"
#include <stack>
class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate);

    vector<Document> AddFindRequest(const string& raw_query,const DocumentStatus status);

    vector<Document> AddFindRequest(const string& raw_query);

    uint64_t GetNoResultRequests() const
    {
        return this->requests_.size();
    }
private:
    struct QueryResult {
        uint64_t timestamp; // время добавления документа
        QueryResult(uint64_t time)
            :timestamp(time) {}
    };
    deque<QueryResult> requests_;
    const SearchServer* search_server_;
    const static int sec_in_day_ = 1440;
    void UpdateRequests(vector<Document>);
};