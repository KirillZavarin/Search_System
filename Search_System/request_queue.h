#pragma once

#include <vector>
#include <deque>

#include "document.h"
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server) : server(search_server)
    {}

    // ñäåëàåì "îá¸ðòêè" äëÿ âñåõ ìåòîäîâ ïîèñêà, ÷òîáû ñîõðàíÿòü ðåçóëüòàòû äëÿ íàøåé ñòàòèñòèêè

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status_);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::vector<Document> found_result;
        bool empty_docs = false;
    };

    std::deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
    const SearchServer& server;
    int NoResultRequests = 0;

    void Push(QueryResult& result);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    QueryResult result;
    result.found_result = server.FindTopDocuments(raw_query, document_predicate);
    result.empty_docs = result.found_result.empty();
    if (requests_.size() < sec_in_day_) {
        Push(result);
    }
    else {
        if (requests_.front().empty_docs) {
            NoResultRequests--;
        }
        requests_.pop_front();
        Push(result);
    }
    return result.found_result;
}