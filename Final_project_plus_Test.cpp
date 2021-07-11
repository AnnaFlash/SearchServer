// Final_project_plus_Test.cpp: определяет точку входа для приложения.
//

#include "process_queries.h"
#include "log_duration.h"
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "Tests.h"
using namespace std;


ostream& operator<<(ostream& out, const Document& document) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}


void PrintMatchDocumentResult(int document_id, const vector<string_view> words, DocumentStatus status) {
    cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast<int>(status) << ", "s
        << "words ="s;
    for (const auto word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
    const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const exception& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(std::execution::par, raw_query)) {
            cout << document <<endl;
        }
    }
    catch (const exception& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query)
{
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;
        for (const int ids : search_server) {
            const auto [words, status] = search_server.MatchDocument(std::execution::par, query, ids);
            PrintMatchDocumentResult(ids, (words), status);
        }
    }
    catch (const exception& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}
void gen_random(string& s, const int len) {
    static const char alphanum[] =
        "A BCDE F G H IJKLMNOPQRSTUVWXYZ"
        "abcdefg h i j k l m n o pqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

int main() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "cat funny and nasty rat"s,
            "funny with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }
    //int n = 25;
    //string s = { "                         " };
    //for (size_t i = 0; i < 10; i++) {
    //    gen_random(s, n);
    //    search_server.AddDocument(i + 6, s, DocumentStatus::ACTUAL, { 1 });
    //}
    search_server.AddDocument(6, "cat", DocumentStatus::ACTUAL, { 1, 2 });
    search_server.FindTopDocuments("fluffy well-groomed cat uncle Styopa"s, [](int document_id, DocumentStatus status, int rating) { return rating > 2; });
    search_server.FindTopDocuments("", DocumentStatus::ACTUAL);
    FindTopDocuments(search_server, "-cat funny -pet nasty rat");
  /* DocumentStatus status = DocumentStatus::ACTUAL;
   search_server.FindTopDocuments("", DocumentStatus::ACTUAL);*/
   /*MatchDocuments(search_server, "cat nasty -hair");
    map<string_view, double> a = search_server.GetWordFrequencies(3);
    for (auto& [words, freq] : a) {
        cout << words << " " << freq << endl;
    }*/
   // const string query = "curly and funny -not"s;
    TestSearchServer();
    return 0;
}