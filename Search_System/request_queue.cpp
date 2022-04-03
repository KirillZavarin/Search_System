#include "request_queue.h"

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status_) {
    return AddFindRequest(raw_query, [status_](int document_id, DocumentStatus status, int rating) { return status == status_; });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    return AddFindRequest(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
}

int RequestQueue::GetNoResultRequests() const {
    return NoResultRequests;
}

void  RequestQueue::Push(QueryResult& result) {
    requests_.push_back(result);
    if (result.empty_docs) {
        NoResultRequests++;
    }
}