#include <algorithm>
#include <execution>
#include "process_queries.h"

//using namespace std;

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    
    std::vector<std::vector<Document>> documents_lists(queries.size());

    std::transform(std::execution::par, queries.begin(), queries.end(),
       documents_lists.begin(), [&](const std::string query) {return search_server.FindTopDocuments(query);});

    return documents_lists;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    
    std::vector<Document> documents_list;

    for (const auto& documents : ProcessQueries(search_server, queries)) {
        documents_list.insert(documents_list.end(), documents.begin(), documents.end());}

    return documents_list;
}
