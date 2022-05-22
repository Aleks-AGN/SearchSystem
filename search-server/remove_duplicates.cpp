#include "remove_duplicates.h"
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>

//using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    using namespace std::literals;
    std::vector<int> duplicated_ids;

    auto it_end = search_server.end();
    for (auto it1 = search_server.begin(); it1 != it_end; ++it1) {
        auto word_freq1 = search_server.GetWordFrequencies(*it1);
        std::set<std::string> words1;
        for (const auto& [key, _]: word_freq1) {
            words1.insert(std::string(key)); 
        }
        
        auto it2_begin = it1;
        ++it2_begin;
        
        if (it2_begin != it_end) {
            for (auto it2 = it2_begin; it2 != it_end; ++it2) {
                auto word_freq2 = search_server.GetWordFrequencies(*it2);
                
                if (word_freq1.size() != word_freq2.size()) {
                    continue;
                }

                std::set<std::string> words2;
                for (const auto& [key, _]: word_freq2) {
                    words2.insert(std::string(key)); 
                }

                if (words1 == words2) {
                    std::cout << "Found duplicate document id "s << *it2 << std::endl;
                    duplicated_ids.push_back(*it2);
                    break;
                }
            }
        }
    }

    for (const auto id : duplicated_ids) {
        search_server.RemoveDocument(id);
    }
}
