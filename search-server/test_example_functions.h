#pragma once

#include <string>
#include <vector>
#include "search_server.h"
#include "remove_duplicates.h"

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings);
void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query);
void MatchDocuments(const SearchServer& search_server, const std::string& query);

void TestSearchServer1();
void TestSearchServer2();
void TestSearchServer3();
