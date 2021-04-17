#include "Tests.h"
using namespace std;
const double epsilon = 1e-6;


template <typename Tfirst, typename Tsecond>
ostream& operator<<(ostream& out, const pair<Tfirst, Tsecond>& container) {
    out << container.first << ": " << container.second;
    return out;
}

template <typename T>

void Print(ostream& out, const T& container) {
    for (const auto& element : container) {
        if (*(--container.end()) == element) { out << element; continue; }
        out << element << ", "s;
    }
}

template <typename T>
ostream& operator<<(ostream& out, const vector<T>& container) {
    out << "[";
    Print(out, container);
    out << "]";
    return out;
}

template <typename T>
ostream& operator<<(ostream& out, const set<T>& container) {
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}
template <typename Tfirst, typename Tsecond>
ostream& operator<<(ostream& out, const map<Tfirst, Tsecond>& container) {
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}

template <typename T>
void RunTestImpl(const T& func, const std::string& func_name) {
    func();
    std::cerr << func_name << " OK!" << std::endl;
}
// -------- Начало модульных тестов поисковой системы ----------
#define RUN_TEST(func)  RunTestImpl((func), __FUNCTION__);
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))


void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))


// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/
void TestMinusWords() {
    const int doc_id1 = 42;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = { 1, 2, 3 };
    const int doc_id2 = 52;
    const string content2 = "cat in the village"s;
    const vector<int> ratings2 = { 3, 4, 5 };
    {
        SearchServer server;
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);

        const auto found_docs1 = server.FindTopDocuments("cat city"s);
        ASSERT_EQUAL(found_docs1.size(), 2);

        const auto found_docs2 = server.FindTopDocuments("cat -city"s);
        ASSERT_EQUAL_HINT(found_docs2.size(), 1, "Not delete document with minus-word"s);

        const auto found_docs3 = server.FindTopDocuments("-cat -city"s);
        ASSERT_HINT(found_docs3.empty(), "All documents with minus words must be deleted");
    }
}

//Матчинг документов. При матчинге документа по поисковому запросу должны быть 
//возвращены все слова из поискового запроса, присутствующие в документе. 
//Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.

void TestMatching() {
    SearchServer search_server;
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });

    const auto [words0, status0] = search_server.MatchDocument("пушистый кот"s, 0);
    ASSERT_EQUAL(words0.size(), 1);
    const auto [words1, status1] = search_server.MatchDocument("пушистый кот"s, 1);
    ASSERT_EQUAL(words1.size(), 2);
    const auto [words2, status2] = search_server.MatchDocument("хвост выразительные глаза -пёс"s, 2);
    ASSERT_HINT(words2.empty(), "Minus word, word list must be empty");
}

//Сортировка найденных документов по релевантности. 
//Возвращаемые при поиске документов результаты должны 
//быть отсортированы в порядке убывания релевантности.
//Корректное вычисление релевантности найденных документов.
void TestRelevance() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    const auto found_docs = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL_HINT(found_docs[0].id, 1, "Wrong sorting order"s);
    ASSERT_EQUAL_HINT(found_docs[1].id, 2, "Wrong sorting order"s);

    ASSERT_HINT((abs(found_docs[0].relevance - 0.650672) < epsilon), "Incorrect calculation of relevance"s);
    ASSERT_HINT((abs(found_docs[1].relevance - 0.274653) < epsilon), "Incorrect calculation of relevance"s);
}

//Вычисление рейтинга документов.
//Рейтинг добавленного документа равен 
//среднему арифметическому оценок документа.

void TestRating() {
    const int doc_id = 42;
    const string doc42 = "cat in the city"s;
    const vector<int> ratings = { 3, 9, 8 };
    const int doc_id2 = 52;
    const string doc52 = "cat on the moon with cat"s;
    const vector<int> ratings2 = { 13, 5, 18 };
    const int doc_id3 = 22;
    const string doc22 = "journey to the center of the earth"s;
    const vector<int> ratings3 = { 23, 11, 9 };
    map<int, vector<int>> id_rating = { {42,ratings},
                                    {52, ratings2},
                                    {22,ratings3} };
    SearchServer server;
    server.AddDocument(doc_id, doc42, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, doc52, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(doc_id3, doc22, DocumentStatus::ACTUAL, ratings3);
    const auto found_docs = server.FindTopDocuments("moon city earth"s);
    for (const auto& f : found_docs) {
        int avrating = 0;
        for (auto rate : id_rating.at(f.id)) {
            avrating += rate;
        }
        avrating /= static_cast<double> (id_rating.at(f.id).size());
        ASSERT_EQUAL_HINT(f.rating, avrating, "Incorrect rating calculation"s);
    }
}

//Фильтрация результатов поиска с использованием предиката, 
//задаваемого пользователем.
//Поиск документов, имеющих заданный статус.
void TestPredicate() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    search_server.AddDocument(4, "дядя Стёпа милиционер"s, DocumentStatus::BANNED, { 19,2 });
    set <int> actual;
    set <int> actual_input = { 0,1,2 };
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот дядя Стёпа"s, DocumentStatus::ACTUAL)) {
        actual.insert(document.id);
    }
    ASSERT_EQUAL_HINT(actual, actual_input, "Invalid sampling by ACTUAL status"s);
    set <int> banned;
    set <int> banned_input = { 3,4 };
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот дядя Стёпа"s, DocumentStatus::BANNED)) {
        banned.insert(document.id);
    }
    ASSERT_EQUAL_HINT(banned, banned_input, "Invalid sampling by user status"s);
    set <int> even;
    set <int> even_input = { 0,2,4 };
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот дядя Стёпа"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        even.insert(document.id);
    }
    ASSERT_EQUAL_HINT(even, even_input, "Invalid sampling by id"s);
    set <int> rate;
    set <int> rate_input = { 1,3,4 };
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот дядя Стёпа"s, [](int document_id, DocumentStatus status, int rating) { return rating > 2; })) {
        rate.insert(document.id);
    }
    ASSERT_EQUAL_HINT(rate, rate_input, "Invalid sampling by ratings"s);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMatching);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestRating);
    RUN_TEST(TestRelevance);
    // Не забудьте вызывать остальные тесты здесь
}
