#include "request_queue.h"

//using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server), total_request_count_(0), empty_request_count_(0)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document> documents = search_server_.FindTopDocuments(raw_query, status);
    AddRequestToDegue(documents);
    DeleteRequestFromDeque(documents);
    return documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> documents = search_server_.FindTopDocuments(raw_query);
    AddRequestToDegue(documents);
    DeleteRequestFromDeque(documents);
    return documents;
}

int RequestQueue::GetNoResultRequests() const {
    return empty_request_count_;
}

void RequestQueue::AddRequestToDegue(std::vector<Document> documents) {
    ++total_request_count_;
    if (documents.empty()) {
        ++empty_request_count_;
    }
    requests_.push_back({total_request_count_, documents.empty()});
}

void RequestQueue::DeleteRequestFromDeque(std::vector<Document> documents) {        
    if (requests_.size() > min_in_day_) {
        if (!documents.empty()) {
            --empty_request_count_;
        }
        requests_.pop_front();
    }
}
