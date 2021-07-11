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

vector<string_view> SplitIntoWordsView(string_view str) {
    vector<string_view> result;
    for (size_t pos = 0; pos != str.npos; str.remove_prefix(pos + 1)) {
        pos = str.find(' ');
        result.push_back(str.substr(0, pos));
    }
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
SearchServer::SearchServer(const string_view stop_words_text)
    : SearchServer(SplitIntoWordsView(stop_words_text))  // Invoke delegating constructor from string container
{
    for (const auto& stop_word : stop_words_) {
        if (!IsValidWord(stop_word)) {
            throw invalid_argument("wrong stop words"s);
        }
    }
}
void SearchServer::AddDocument(int document_id, const string_view document,
    const DocumentStatus& status, const vector<int>& ratings)
{
    if (document_id < 0) {
        throw invalid_argument("try to add document with negative id");
    }
    if (documents_.count(document_id) > 0) {
        throw invalid_argument("duplicate id");
    }

    const vector<string> words = SplitIntoWordsNoStop(document);
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

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view raw_query, int document_id) const
{
    const Query query = ParseQuery(raw_query);
    vector<string_view> matched_words;
    if (word_to_document_freqs_.count(document_id)) {
        for (const auto& word : query.minus_words) {
            if ((word_to_document_freqs_.at(document_id).count(string(word))) == 0) {
                continue;
            }
            matched_words.clear();
            return { matched_words, documents_.at(document_id).status };
        }
        for (const auto& word : query.plus_words) {
            auto& word_freqs = word_to_document_freqs_.at(document_id);
            auto it = word_freqs.find(string(word));
            if (it != word_freqs.end()) {
                matched_words.push_back(it->first);
            }
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy seq, const string_view raw_query, int document_id) const
{
    return MatchDocument(raw_query, document_id);
}
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy par, const string_view raw_query, int document_id) const
{
    const Query query = ParseQuery(par, raw_query);
    vector<string_view> matched_words;
    if (word_to_document_freqs_.count(document_id)) {
       const map<string, double>& word_freqs = word_to_document_freqs_.at(document_id);
        std::for_each(par, query.plus_words.begin(), query.plus_words.end(), [&](auto& word) 
            {
                auto it = word_freqs.find(word);
                if (it != word_freqs.end())
                    matched_words.push_back((it->first));
            });

        if (std::any_of(par, query.minus_words.begin(), query.minus_words.end(), [&](auto& word) {
            return word_freqs.count(word);
            }))
        {
            matched_words.clear();
        }
    }
    return { matched_words, documents_.at(document_id).status };
}
//private

const map<string_view, double> SearchServer::GetWordFrequencies(int document_id) const
{
    map<string_view, double> res;
    for (const auto& [word, freq] : word_to_document_freqs_.at(document_id)) {
        res[word] = freq;
    }
    //res = word_to_document_freqs_.at(document_id);
    return res;
}

void SearchServer::RemoveDocument(int document_id)
{
    word_to_document_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}
void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    RemoveDocument(document_id);
}
void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    RemoveDocument(document_id);
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
vector<string> SearchServer::SplitIntoWordsNoStop(const string_view text) const
{
    vector<string> words;
    for (const string_view word : SplitIntoWordsView(text)) {
        if (!IsValidWord(string(word))) {
            throw invalid_argument("Spec symvol in stop words");
        }
        if (!IsStopWord(string(word))) {
            words.push_back(string(word));
        }
    }
    return words;
}


vector<Document> SearchServer::FindTopDocuments(string_view raw_query, const DocumentStatus& status) const
{
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus statuss, int rating)
        { return statuss == status; });
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const
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
SearchServer::Query SearchServer::ParseQuery(const string_view text) const
{
    Query query;
    for (const string_view word : SplitIntoWordsView(text)) {
        const QueryWord query_word = ParseQueryWord(string(word));
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
SearchServer::Query SearchServer::ParseQuery(std::execution::sequenced_policy, const string_view text) const {
    return ParseQuery(text);
}
SearchServer::Query SearchServer::ParseQuery(std::execution::parallel_policy, const string_view text) const
{
    Query query;
    vector<string_view> words = SplitIntoWordsView(text);
    vector<QueryWord> query_words(words.size());
    transform(execution::par, words.begin(), words.end(), query_words.begin(), [&](auto& w) {return ParseQueryWord(string(w)); });
    for (auto& query_word : query_words) {
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



