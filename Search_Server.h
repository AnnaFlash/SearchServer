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
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
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
    vector<Document> FindTopDocuments(const string_view raw_query, const DocumentStatus& status) const;
    vector<Document> FindTopDocuments(const string_view raw_query) const;

    template <typename Func>
    vector<Document> FindTopDocuments(const string_view raw_query, const Func& func) const
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
    size_t GetDocumentCount() const
    {
        return documents_.size();
    }
private:
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
        set<string> plus_words;
        set<string> minus_words;
    };
    Query ParseQuery(const string_view text) const;
    Query ParseQuery(std::execution::parallel_policy,const string_view text) const;

    template <typename Func>
    vector<Document>FindAllDocuments(const Query& query, const Func& func) const
    {
        map<int, double> document_to_relevance;
        map<string, int> id_idf;
        map<int, map<string, double>> id_tdf;
        // prepare tf-idf
        for (const auto& id_w_fr : word_to_document_freqs_) {
            for (const string& word : query.plus_words) {
                if (id_w_fr.second.count(word)) {
                    id_idf[word] += 1;
                    id_tdf[id_w_fr.first][word] = id_w_fr.second.at(word);
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
            for (const string& word : query.minus_words) {
                if (id_w_fr.second.count(word)) {
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
};

        //for (const auto& [document_id, relevance] : document_to_relevance) {
        //    matched_documents.push_back({
        //        document_id,
        //        relevance,
        //        documents_.at(document_id).rating
        //        });
        //}