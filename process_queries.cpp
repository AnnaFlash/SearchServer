//#include "process_queries.h"
//using namespace std;
//vector<vector<Document>> ProcessQueries(
//    const SearchServer& search_server,
//    const vector<string>& queries) {
//    vector<vector<Document>> result(queries.size());
//    transform(execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](auto querie)
//        {return search_server.FindTopDocuments(querie); });
//    return result;
//}