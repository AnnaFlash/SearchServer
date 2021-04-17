#pragma once
#include <algorithm>
#include <cmath>
#include <utility>
#include <numeric>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <iostream>

using namespace std;
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double epsilon = 1e-6;
struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
class SearchServer {
public:
	void SetStopWords(const string& text);
	void AddDocument(int document_id, const string& document, const DocumentStatus& status, const vector<int>& ratings);

    int GetDocumentCount() const;
    vector<Document> FindTopDocuments(const string& raw_query, const DocumentStatus& status) const;
    vector<Document> FindTopDocuments(const string& raw_query) const;
    template <typename Func>
    vector<Document> FindTopDocuments(const string& raw_query, const Func& func) const
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
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const;
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const;
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
    vector<Document>FindAllDocuments(const Query& query, const Func& func) const {
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