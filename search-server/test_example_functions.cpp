#include "test_example_functions.h"
#include <iostream>
//#include <stdexcept>
//#include <utility>
#include "read_input_functions.h"
#include "paginator.h"
#include "request_queue.h"
#include "process_queries.h"
#include "log_duration.h"

//using namespace std;

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings) {
    using namespace std::literals;

    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const std::invalid_argument& e) {
        std::cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    using namespace std::literals;

    std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;

    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const std::invalid_argument& e) {
        std::cout << "Ошибка поиска: "s << e.std::logic_error::what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    using namespace std::literals;

    try {
        std::cout << "Матчинг документов по запросу: "s << query << std::endl;
        
        for (const int document_id : search_server) {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            
            std::vector<std::string> string_words(words.size());
            std::transform(words.begin(), words.end(),
            string_words.begin(), [](const std::string_view word) {return std::string(word);});
            PrintMatchDocumentResult(document_id, string_words, status);

            //PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const std::invalid_argument& e) {
        std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.std::logic_error::what() << std::endl;
    }
}

void TestSearchServer1() {
    using namespace std::literals;

    std::cout << std::endl << "--- Start The Test 1 of The SearchServer ---"s << std::endl;
    
    SearchServer search_server("и в на"s);

    {
        LOG_DURATION("Operation time"s);
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(2, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2, 3});
        search_server.AddDocument(3, "большой кот модный ошейник "s, DocumentStatus::ACTUAL, {1, 2, 8});
        search_server.AddDocument(4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
        search_server.AddDocument(5, "большой пёс скворец василий"s, DocumentStatus::ACTUAL, {1, 1, 1});
    }
        
    {   
        LOG_DURATION_STREAM("Operation time"s,  std::cout);
        MatchDocuments(search_server, "пушистый -пёс"s);
    }

    {   
        LOG_DURATION_STREAM("Operation time"s,  std::cout);
        FindTopDocuments(search_server, "пушистый -кот"s);
    }

    const auto search_results = search_server.FindTopDocuments("пушистый пёс"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);

    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        std::cout << *page << std::endl;
        std::cout << "Разрыв страницы"s << std::endl;
    }
    std::cout << "--- End The Test 1 of The SearchServer ---"s << std::endl << std::endl;
}

void TestSearchServer2() {
    using namespace std::literals;

    std::cout << std::endl << "--- Start The Test 2 of The SearchServer ---"s << std::endl;
    
    SearchServer search_server("and in at"s);

    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
    
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
    
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
    
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;

    std::cout << "--- End The Test 2 of The SearchServer ---"s << std::endl << std::endl;
}
/*
void TestSearchServer3() {
    using namespace std::literals;

    std::cout << std::endl << "--- Start The Test 3 of The SearchServer ---"s << std::endl;

    SearchServer search_server("and with"s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // дубликат документа 2, будет удалён
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // отличие только в стоп-словах, считаем дубликатом
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, считаем дубликатом документа 1
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // добавились новые слова, дубликатом не является
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

    // есть не все слова, не является дубликатом
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

    // слова из разных документов, не является дубликатом
    AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    
    
    std::cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
    RemoveDuplicates(search_server);
    std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
    
    
    std::cout << std::endl << "Word frequencies in the first document:" << std::endl;
    const std::map<std::string, double>& word_frequencies1 = search_server.GetWordFrequencies(1);
    for (const auto& [word, frequencies] : word_frequencies1) {
        std::cout << word << " - " << frequencies << std::endl;
    }
    
    std::cout << std::endl << "Word frequencies in the second document:" << std::endl;
    const std::map<std::string, double>& word_frequencies2 = search_server.GetWordFrequencies(2);
    for (const auto& [word, frequencies] : word_frequencies2) {
        std::cout << word << " - " << frequencies << std::endl;
    }

    search_server.RemoveDocument(1);
    search_server.RemoveDocument(1);

    std::cout << "After document with id = 1 removed: "s << search_server.GetDocumentCount() << std::endl;

    std::cout << std::endl << "Word frequencies in the first document:" << std::endl;
    const std::map<std::string, double>& word_frequencies3 = search_server.GetWordFrequencies(1);
    for (const auto& [word, frequencies] : word_frequencies3) {
        std::cout << word << " - " << frequencies << std::endl;
    }
    
    std::cout << "--- End The Test 3 of The SearchServer ---"s << std::endl << std::endl;
}*/

void TestSearchServer4() {
    using namespace std::literals;

    std::cout << std::endl << "--- Start The Test 4 of The SearchServer ---"s << std::endl;

    SearchServer search_server("and with"s);

    int id = 0;

    for (const std::string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s, }) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const std::vector<std::string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };

    id = 0;

    for (const auto& documents : ProcessQueries(search_server, queries)) {
        std::cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << std::endl;
    }

    std::cout << "--- End The Test 4 of The SearchServer ---"s << std::endl << std::endl;
}

void TestSearchServer5() {
    using namespace std::literals;

    std::cout << std::endl << "--- Start The Test 5 of The SearchServer ---"s << std::endl;

    SearchServer search_server("and with"s);

    int id = 0;

    for (const std::string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s, }) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const std::vector<std::string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };

    for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
        std::cout << "Document "s << document.id << " matched with relevance "s << document.relevance << std::endl;
    }

    std::cout << "--- End The Test 5 of The SearchServer ---"s << std::endl << std::endl;
}

void TestSearchServer6() {
    using namespace std::literals;

    std::cout << std::endl << "--- Start The Test 6 of The SearchServer ---"s << std::endl;

    SearchServer search_server("and with"s);

    int id = 0;

    for (const std::string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s, }) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const std::string query = "curly and funny"s;

    auto report = [&search_server, &query] {
        std::cout << search_server.GetDocumentCount() << " documents total, "s
            << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << std::endl;
    };

    report();
    // однопоточная версия
    search_server.RemoveDocument(5);
    report();
    // однопоточная версия
    search_server.RemoveDocument(std::execution::seq, 1);
    report();
    // многопоточная версия
    search_server.RemoveDocument(std::execution::par, 2);
    report();

    std::cout << "--- End The Test 6 of The SearchServer ---"s << std::endl << std::endl;
}

void TestSearchServer7() {
    using namespace std::literals;

    std::cout << std::endl << "--- Start The Test 7 of The SearchServer ---"s << std::endl;

    SearchServer search_server("and with"s);

    int id = 0;

    for (const std::string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s, }) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const std::string query = "curly and funny -not"s;

    {
        const auto [words, status] = search_server.MatchDocument(query, 1);
        std::cout << words.size() << " words for document 1"s << std::endl;
        // 1 words for document 1
    }

    {
        const auto [words, status] = search_server.MatchDocument(std::execution::seq, query, 2);
        std::cout << words.size() << " words for document 2"s << std::endl;
        // 2 words for document 2
    }

    {
        const auto [words, status] = search_server.MatchDocument(std::execution::par, query, 3);
        std::cout << words.size() << " words for document 3"s << std::endl;
        // 0 words for document 3
    }

    std::cout << "--- End The Test 7 of The SearchServer ---"s << std::endl << std::endl;
}

void TestSearchServer8() {
    using namespace std::literals;

    std::cout << std::endl << "--- Start The Test 8 of The SearchServer ---"s << std::endl;

    SearchServer search_server("and with"s);

    int id = 0;

    for (const std::string& text : {
            "white cat and yellow hat"s,
            "curly cat curly tail"s,
            "nasty dog with big eyes"s,
            "nasty pigeon john"s, }) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    std::cout << "ACTUAL by default:"s << std::endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
        PrintDocument(document);
    }
    std::cout << "BANNED:"s << std::endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments(std::execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    std::cout << "Even ids:"s << std::endl;
    // параллельная версия
    for (const Document& document : search_server.FindTopDocuments(std::execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    std::cout << "--- End The Test 8 of The SearchServer ---"s << std::endl << std::endl;
}
