#pragma once
#include "search_server.h"
void RemoveDuplicates(SearchServer& search_server) {
    set<int> badids;
    set<string> ids_string;
    set<string> jds_string;
    for (auto ids = search_server.begin(); ids != search_server.end(); ids++) {
        for (auto& _ : search_server.GetWordFrequencies(*ids)) {
            ids_string.insert(_.first);
        }
        for (auto jds = ids; jds != search_server.end(); jds++) {
            if (ids == jds) { continue; }
            if (search_server.GetWordFrequencies(*jds).size() > ids_string.size()) { break; }
            for (auto& _ : search_server.GetWordFrequencies(*jds)) {
                jds_string.insert(_.first);
            }
            if (ids_string == jds_string) {
                badids.insert((*ids > *jds ? *ids : *jds));
            }
            jds_string.clear();
        }
        ids_string.clear();
    }
    for (const int& ids : badids) {
        cout << "Found duplicate document id " << ids << endl;
        search_server.RemoveDocument(ids);
    }
}