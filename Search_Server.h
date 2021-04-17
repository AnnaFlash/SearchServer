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
	vector<Document> FindTopDocuments(const string & raw_query, const DocumentStatus & status) const;
	vector<Document> FindTopDocuments(const string& raw_query) const;
    int GetDocumentCount() const;
	template <typename Func>
	vector<Document> FindTopDocuments(const string& raw_query, const Func& func) const;
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
    vector<Document> FindAllDocuments(const Query& query, const Func& func) const;

};