#include "Search_Server.h"

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text)
{
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            words.push_back(word);
            word = "";
        }
        else {
            word += c;
        }
    }
    words.push_back(word);

    return words;
}
SearchServer::SearchServer()
{
}
// 
SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
    for (const auto& stop_word : stop_words_) {
        if (!IsValidWord(stop_word)) {
            throw invalid_argument("wrong stop words"s);
        }
    }
}
void SearchServer::AddDocument(int document_id, const string& document,
    const DocumentStatus& status, const vector<int>& ratings)
{

    if (document_id < 0) {
        throw invalid_argument("try to add document with negative id");
    }
    if (documents_.count(document_id) > 0) {
        throw invalid_argument("duplicate id");
    }

    const vector<string> words = SplitIntoWordsNoStop(document);
    // check if we add a duplicate
    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
        word_to_document_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id,
        DocumentData{
            ComputeAverageRating(ratings),
            status
        });
    document_ids_.insert(document_id);
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const
{
    const Query query = ParseQuery(raw_query);
    vector<string> matched_words;
    if (word_to_document_freqs_.count(document_id)) {
        for (const string& word : query.plus_words) {
            if ((word_to_document_freqs_.at(document_id).count(word)) == 0) {
                continue;
            }
            matched_words.push_back(word);
        }
        for (const string& word : query.minus_words) {
            if ((word_to_document_freqs_.at(document_id).count(word)) == 0) {
                continue;
            }
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

//private

const map<string, double> SearchServer::GetWordFrequencies(int document_id) const
{
    map<string, double> res;
    res = word_to_document_freqs_.at(document_id);
    return res;
}

void SearchServer::RemoveDocument(int document_id)
{
    word_to_document_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

bool SearchServer::IsStopWord(const string& word) const
{
    return stop_words_.count(word) > 0;
}
bool SearchServer::IsValidWord(const string& word)
{
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}
vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const
{
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Spec symvol in stop words");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}


vector<Document> SearchServer::FindTopDocuments(const string& raw_query, const DocumentStatus& status) const
{
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus statuss, int rating)
        { return statuss == status; });
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const
{
    return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating)
        { return status == DocumentStatus::ACTUAL; });
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings)
{
    return ratings.size() > 0 ? (accumulate(ratings.begin(), ratings.end(), 0)
        / static_cast<int>(ratings.size())) : 0;
}
SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const
{
    if (text.empty()) {
        throw invalid_argument("Empty query"s);
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
        if (text.empty()) {
            throw invalid_argument("Minus word can't be empty"s);
        }
    }
    if (text[0] == '-') {
        throw invalid_argument("Double minus in minus word"s);
    }
    if (!IsValidWord(text)) {
        throw invalid_argument("Spec symvol"s);
    }
    return {
        text,
        is_minus,
        IsStopWord(text)
    };
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const
{
    Query query;
    for (const string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            }
            else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}



