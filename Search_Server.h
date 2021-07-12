#pragma once
#include "headers.h"
#include "Log_duration.h"
using namespace std;
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double epsilon = 1e-6;
struct Document {
    int id;
    double relevance;
    int rating;
    Document() {
        id = 0;
        relevance = 0.0;
        rating = 0;
    }
    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings)
{
    set<string> non_empty_strings;
    for (const auto& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(string(str));
        }
    }
    return non_empty_strings;
}

class SearchServer {
public:
    // Defines an invalid document id
// You can refer this constant as SearchServer::INVALID_DOCUMENT_ID
    inline static constexpr int INVALID_DOCUMENT_ID = -1;
    template<typename TError>
    void ErrorThrowProcessing(const TError& e, string error_type) const
    {
        cout << error_type << ": "s << e.what() << endl;
    }

    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        for (const auto& stop_word : stop_words_) {
            if (!IsValidWord(stop_word)) {
                throw invalid_argument("wrong stop words"s);
            }
        }
    }
    explicit SearchServer();
    explicit SearchServer(const string& stop_words_text);
    explicit SearchServer(const string_view stop_words_text);
    void AddDocument(int document_id, const string_view document, const DocumentStatus& status, const vector<int>& ratings);

    template <typename Func>
    vector<Document> FindTopDocuments(string_view raw_query, const Func& func) const
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, func);
        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < epsilon) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const {
        if constexpr (is_same_v<ExecutionPolicy, execution::sequenced_policy>) {
            return FindTopDocumentsSeq(raw_query, document_predicate);
        }
        else {
            return FindTopDocumentsPar(policy, raw_query, document_predicate);
        }
    }

    vector<Document> FindTopDocuments(string_view raw_query, const DocumentStatus& status) const;//**

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query,const DocumentStatus status) const { //**
        if constexpr (is_same_v<ExecutionPolicy, execution::sequenced_policy>) {
            return FindTopDocuments(raw_query, status);
        }
        else {
            return FindTopDocumentsPar(policy, raw_query, [status](int document_id, DocumentStatus statuss, int rating)
                { return statuss == status; });
        }
    }

    vector<Document> FindTopDocuments(string_view raw_query) const; //*

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy& policy, std::string_view raw_query) const { //*
        if constexpr (is_same_v<ExecutionPolicy, execution::sequenced_policy>) {
            return FindTopDocuments(raw_query);
        }
        else {
            return FindTopDocumentsPar(policy, raw_query, [](int document_id, DocumentStatus status, int rating)
                { return status == DocumentStatus::ACTUAL; });
        }
    }
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocumentsPar(const std::execution::parallel_policy& par, std::string_view raw_query, DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(par, raw_query);
        auto matched_documents = FindAllDocuments(par, query, document_predicate);
        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < epsilon) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
      
    tuple<vector<string_view>, DocumentStatus> MatchDocument(const string_view raw_query, int document_id) const;
    tuple<vector<string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy seq, const string_view raw_query, int document_id) const;
    tuple<vector<string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy par, const string_view raw_query, int document_id) const;
    auto begin()
    {
        return document_ids_.begin();
    }

    auto end()
    {
        return document_ids_.end();
    }

    auto begin() const
    {
        return document_ids_.begin();
    }

    auto end() const
    {
        return document_ids_.end();
    }
    const map<string_view, double> GetWordFrequencies(int document_id) const;
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
    size_t GetDocumentCount() const
    {
        return documents_.size();
    }
private:
    mutable mutex global_mutex;
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    set<string> stop_words_;
    map<int, map<string, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    set<int> document_ids_;
    bool IsStopWord(const string& word) const;
    static bool IsValidWord(const string& word);
    vector<string> SplitIntoWordsNoStop(const string_view text) const;
    static int ComputeAverageRating(const vector<int>& ratings);
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
    QueryWord ParseQueryWord(string text) const;
    struct Query {
        set<string, less<>> plus_words;
        set<string, less<>> minus_words;
    };
    Query ParseQuery(const string_view text) const;
    Query ParseQuery(std::execution::parallel_policy,const string_view text) const;
    Query ParseQuery(std::execution::sequenced_policy, const string_view text) const;

    template <typename Func>
    vector<Document> FindAllDocuments(const Query& query, const Func& func) const
    {
        lock_guard<mutex> guard(global_mutex);
        map<int, double> document_to_relevance;
        map<string_view, int> id_idf;
        map<int, map<string_view, double>> id_tdf;
        // prepare tf-idf
        for (const auto& id_w_fr : word_to_document_freqs_) {
            for (const auto& word : query.plus_words) {
                if (id_w_fr.second.count(string(word))) {
                    id_idf[word] += 1;
                    id_tdf[id_w_fr.first][word] = id_w_fr.second.at(string(word));
                }
            }

        }
        for (const auto& [document_id, w_fr] : id_tdf) {
            if (func(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                for (const auto& word : w_fr) {
                    document_to_relevance[document_id] +=
                        (static_cast<double>(log(document_ids_.size() * 1.0 / static_cast<double>(id_idf.at(word.first)))) * word.second);
                }
            }
        }
        // erase documents with minus words
        int badid = -1;
        for (const auto& id_w_fr : word_to_document_freqs_) {
            for (const auto& word : query.minus_words) {
                if (id_w_fr.second.count(string(word))) {
                    badid = id_w_fr.first;
                    break;
                }
            }
            badid >= 0 ? document_to_relevance.erase(badid) : badid = -1;
        }
        vector<Document> matched_documents(document_to_relevance.size());
        transform(std::execution::par, document_to_relevance.begin(), document_to_relevance.end(), matched_documents.begin(),
            [&](pair<int, double> id_rel) {
                return Document(id_rel.first, id_rel.second, documents_.at(id_rel.first).rating);
            });
        return matched_documents;
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const {
        return FindAllDocuments(query, document_predicate);
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy& par, const Query& query, DocumentPredicate document_predicate) const {
        lock_guard<mutex> guard(global_mutex);
        map<int, double> document_to_relevance;
        map<string_view, int> id_idf;
        map<int, map<string_view, double>> id_tdf;
        // prepare tf-idf
        std::mutex local_mutex;
        for_each(par, word_to_document_freqs_.begin(), word_to_document_freqs_.end(), [&](const auto& id_w_fr) 
            {
                for_each(par, query.plus_words.begin(), query.plus_words.end(), [&](const auto& word) 
                    {
                        if (id_w_fr.second.count(string(word))) {
                            lock_guard<mutex> local_guard(local_mutex);
                            id_idf[word] += 1;
                            id_tdf[id_w_fr.first][word] = id_w_fr.second.at(string(word));
                        }
                    });
            });  

        for_each(par, id_tdf.begin(), id_tdf.end(), [&](const auto& id_w_fr) 
            {
                if (document_predicate(id_w_fr.first, documents_.at(id_w_fr.first).status, documents_.at(id_w_fr.first).rating)) {
                    for_each(par, id_w_fr.second.begin(), id_w_fr.second.end(), [&](const auto& word)
                        {
                            lock_guard<mutex> local_guard(local_mutex);
                            document_to_relevance[id_w_fr.first] +=
                                (static_cast<double>(log(document_ids_.size() * 1.0 / static_cast<double>(id_idf.at(word.first)))) * word.second);
                        });
                }
            });
        // erase documents with minus words

        for_each(par, word_to_document_freqs_.begin(), word_to_document_freqs_.end(), [&](const auto& id_w_fr) 
            {
                int badid = -1;
                for_each(par, query.minus_words.begin(), query.minus_words.end(), [&](const auto& word) 
                    {
                        if (id_w_fr.second.count(string(word))) {
                            badid = id_w_fr.first;
                        }
                    });
                badid >= 0 ? document_to_relevance.erase(badid) : badid = -1;
            });
        vector<Document> matched_documents(document_to_relevance.size());
        transform(par, document_to_relevance.begin(), document_to_relevance.end(), matched_documents.begin(),
            [&](pair<int, double> id_rel) {
                return Document(id_rel.first, id_rel.second, documents_.at(id_rel.first).rating);
            });
        return matched_documents;
    }
};