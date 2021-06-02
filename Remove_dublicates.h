#pragma once
#include "search_server.h"
void RemoveDuplicates(SearchServer& search_server) {
    set<int> badids;
    set<set<string>> words;
    set<string> doc;
    map<string, double> w_freq;
    for (const int& ids : search_server) {
        w_freq = search_server.GetWordFrequencies(ids);
        transform(w_freq.begin(), w_freq.end(),
            inserter(doc, begin(doc)), [](const auto& arg) {return arg.first; });
        if (words.count(doc) != 0) {
            badids.insert(ids);
        }
        else {
            words.insert(doc);
        }
        doc.clear();
    }

    for (const int& ids : badids) {
        cout << "Found duplicate document id " << ids << endl;
        search_server.RemoveDocument(ids);
    }
}