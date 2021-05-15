#pragma once
#include "headers.h"
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
	void AddDocument(int document_id, const string& document, const DocumentStatus& status, const vector<int>& ratings);
    int GetDocumentCount() const;
    int GetDocumentId(int index) const;
    vector<Document> FindTopDocuments(const string& raw_query, const DocumentStatus& status) const;
    vector<Document> FindTopDocuments(const string& raw_query) const;

    template <typename Func>
    vector<Document> FindTopDocuments(const string& raw_query, const Func& func) const
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, func);
        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs)  {
                if (abs(lhs.relevance - rhs.relevance) < epsilon)  {
                    return lhs.rating > rhs.rating;
                }
                else  {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)  {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const;
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> document_ids_;
    bool IsStopWord(const string& word) const;
    static bool IsValidWord(const string& word);
    vector<string> SplitIntoWordsNoStop(const string& text) const;
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
    Query ParseQuery(const string& text) const;
    double ComputeWordInverseDocumentFreq(const string& word) const;
    template <typename Func>
    vector<Document>FindAllDocuments(const Query& query, const Func& func) const 
    {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (func(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
                });
        }
        return matched_documents;
    }
};
