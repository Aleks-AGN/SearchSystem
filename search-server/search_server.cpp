#include "search_server.h"
#include <cmath>
#include <cassert>

//using namespace std;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
                                                         // from string container
{
}

SearchServer::SearchServer(std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
                                                         // from string container
{
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    using namespace std::string_literals;
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    
    std::transform(words.begin(), words.end(), words.begin(), [this](auto word) {
        auto [it, _] = document_words_.insert(std::string(word));
        return std::string_view(*it);});

    const double inv_word_count = 1.0 / words.size();
    for (const auto word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.push_back(document_id);
}

void SearchServer::RemoveDocument(int document_id) {

    auto it = find(document_ids_.begin(), document_ids_.end(), document_id);

    if (it == document_ids_.end()) {
        using namespace std::literals;
        std::cout << "Document id = "s << document_id << " doesn't exist." << std::endl;
        return;
    }  
    
    std::vector<std::string_view> words_to_delete;
    for (auto& [key, value] : word_to_document_freqs_) {
        value.erase(document_id);
        if (value.empty()) {
            words_to_delete.push_back(key);
        }
    }

    for (auto& word : words_to_delete) {
        word_to_document_freqs_.erase(word);
    }

    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(it);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id) {
    
    auto it = std::find(policy, document_ids_.begin(), document_ids_.end(), document_id);

    if (it == document_ids_.end()) {
        return;
    }
    
    const std::map<std::string_view, double>& m = GetWordFrequencies(document_id);
    std::vector<const std::string_view*> v(m.size());
    
    std::transform(policy, m.begin(), m.end(), v.begin(),
        [](auto& str_to_freq) {
            return &str_to_freq.first;});

    std::for_each(policy, v.begin(), v.end(),
        [this, document_id](const auto ptr) {
            word_to_document_freqs_[*ptr].erase(document_id);});

    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(it);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy,
const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy,
const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy,
const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy,
const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::vector<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::vector<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(const std::string_view& raw_query, int document_id) const {
    
    assert(documents_.count(document_id) > 0);
    
    const auto word_freqs = document_to_word_freqs_.at(document_id);
    
    const auto query = ParseQuery(raw_query);

    std::vector<std::string_view> matched_words(query.plus_words.size());
                     
    auto last_copy = std::copy_if(query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
        [&word_freqs](const auto word) {return word_freqs.count(word) > 0;});

    matched_words.erase(last_copy, matched_words.end());
    std::sort(matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(matched_words.begin(), matched_words.end()), matched_words.end());
    
    if (std::any_of(query.minus_words.begin(), query.minus_words.end(),
        [&word_freqs](const auto word) {return word_freqs.count(word) > 0;})) {
            matched_words.clear();
            return {matched_words, documents_.at(document_id).status};
    }

    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(const std::execution::sequenced_policy& policy, const std::string_view& raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(const std::execution::parallel_policy& policy, const std::string_view& raw_query, int document_id) const {
    
    assert(documents_.count(document_id) > 0);
    
    const auto& word_freqs = document_to_word_freqs_.at(document_id);
    
    const auto query = ParseQuery(policy, raw_query);

    std::vector<std::string_view> matched_words(query.plus_words.size());

    if (std::any_of(policy, query.minus_words.begin(), query.minus_words.end(),
        [&word_freqs](const auto word) {return word_freqs.count(word) > 0;})) {
            matched_words.clear();
            return {matched_words, documents_.at(document_id).status};
    }
                     
    auto last_copy = std::copy_if(policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
        [&word_freqs](const auto word) {return word_freqs.count(word) > 0;});

    matched_words.erase(last_copy, matched_words.end());
    std::sort(policy, matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(policy, matched_words.begin(), matched_words.end()), matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {

    if (document_to_word_freqs_.find(document_id) == document_to_word_freqs_.end()) {
        static std::map<std::string_view, double> m;
        return m;
    } else {
        return document_to_word_freqs_.at(document_id);
    }
}

bool SearchServer::IsStopWord(const std::string_view word) const {

    auto it = stop_words_.find(word);
    return (it != stop_words_.end());
}

bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    using namespace std::string_literals;
    std::vector<std::string_view> words;
    for (const auto word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }

    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {
    using namespace std::string_literals;
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + std::string(text) + " is invalid");
    }
    return {word, is_minus, IsStopWord(word)};
}
 
SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    SearchServer::Query result;
    for (const auto word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    std::sort(result.minus_words.begin(), result.minus_words.end());
    auto it_last_minus_words = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(it_last_minus_words, result.minus_words.end());
        
    std::sort(result.plus_words.begin(), result.plus_words.end());
    auto it_last_plus_word = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(it_last_plus_word, result.plus_words.end());

    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::sequenced_policy& policy, const std::string_view text) const {
    SearchServer::Query result;
    for (const auto word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    
    std::sort(result.minus_words.begin(), result.minus_words.end());
    auto it_last_minus_words = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(it_last_minus_words, result.minus_words.end());
        
    std::sort(result.plus_words.begin(), result.plus_words.end());
    auto it_last_plus_word = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(it_last_plus_word, result.plus_words.end());
    
    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy& policy, const std::string_view text) const {
    SearchServer::Query result;
    for (const auto word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
