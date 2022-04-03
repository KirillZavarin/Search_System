#pragma once

#include <vector>
#include <string>
#include <cassert>
#include <set>
#include <map>
#include <algorithm>
#include <numeric>
#include <math.h>
#include <execution>
#include <functional>
#include <string_view>

#include "document.h"
#include "string_processing.h"
#include "log_duration.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    SearchServer() = default;

    SearchServer(const std::string text);

    SearchServer(const std::string_view text);

    template<typename StringCollection>
    SearchServer(const StringCollection& stop_words);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template<typename Predicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, Predicate predicate) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status_) const;

    template< typename ExecutionPolicy, typename Predicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, Predicate predicate) const;

    template<typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query) const;

    template<typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentStatus status_) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy par, const std::string_view& raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy seq, const std::string_view& raw_query, int document_id) const;

    std::set<int>::iterator begin() const;

    std::set<int>::iterator end() const;

    void RemoveDocument(int document_id);

    void RemoveDocument(std::execution::parallel_policy par, int document_id);

    void RemoveDocument(std::execution::sequenced_policy seq, int document_id);

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

private:

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::set<std::string, std::less<>> plus_words;
        std::set<std::string, std::less<>> minus_words;
    };

    std::set<std::string, std::less<>> stop_words_;

    std::map<int, DocumentData> documents_;

    std::map<std::string, std::map<int, double>> word_to_document_freqs_;

    std::map<int, std::map<std::string_view, double>> id_word_to_freqs;

    std::set<int> IDs;

    bool IsStopWord(const std::string_view& word) const;

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string_view text) const;

    Query ParseQuery(const std::string_view& text) const;

    template <typename ExecutionPolicy>
    Query ParseQuery(ExecutionPolicy&& policy, const std::string_view& text) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename Predicate>
    std::vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const;

    template <typename Predicate, typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(ExecutionPolicy&& policy, const Query& query, Predicate predicate) const;

    bool IsValidWord(const std::string& word) const;

    bool IsValidWord(const std::string_view word) const;
};

template<typename StringCollection>
SearchServer::SearchServer(const StringCollection& stop_words) {
    using namespace std::string_literals;
    for (const std::string& word : stop_words) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument(" Invalid symbol in "s + word + " word"s);
        };
        stop_words_.insert(word);
    }
}

template <typename Predicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, Predicate predicate) const {
    using namespace std::string_literals;

    std::map<int, double> document_to_relevance;
    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(as_string(word)) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(as_string(word));
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(as_string(word))) {
            if (predicate(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(as_string(word)) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(as_string(word))) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({
            document_id,
            relevance,
            documents_.at(document_id).rating
            });
    }
    return matched_documents;
}

template <typename Predicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy, const Query& query, Predicate predicate) const {
    using namespace std::string_literals;

    ConcurrentMap<int, double> document_to_relevance(8);

    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](const auto& word) {
        if (word_to_document_freqs_.count(as_string(word)) != 0) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(as_string(word));
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(as_string(word))) {
                if (predicate(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        }
        });

    std::map<int, double> map_document_to_relevance = document_to_relevance.BuildOrdinaryMap();

    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(as_string(word)) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(as_string(word))) {
            map_document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;

    for (const auto [document_id, relevance] : map_document_to_relevance) {
        matched_documents.push_back({
            document_id,
            relevance,
            documents_.at(document_id).rating
            });
    }
    return matched_documents;
}

template<typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, Predicate predicate) const {

    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, predicate);

    std::sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentStatus status_) const {
    return FindTopDocuments(policy, raw_query, [status_](int document_id, DocumentStatus status, int rating) { return status == status_; });
}

template<typename ExecutionPolicy, typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, Predicate predicate) const {

    const Query query = ParseQuery(policy, raw_query);
    auto matched_documents = FindAllDocuments(policy, query, predicate);

    std::sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template<typename ExecutionPolicy>
SearchServer::Query SearchServer::ParseQuery(ExecutionPolicy&& policy, const std::string_view& text) const {
    Query query;

    auto words = SplitIntoWords(text);

    const auto combine_queries = [](Query first, Query second) {
        first.minus_words.insert(make_move_iterator(second.minus_words.begin()), make_move_iterator(second.minus_words.end()));
        first.plus_words.insert(make_move_iterator(second.plus_words.begin()), make_move_iterator(second.plus_words.end()));
        return first;
    };

    return std::transform_reduce(policy, std::make_move_iterator(words.begin()), std::make_move_iterator(words.end()), Query{}, combine_queries, [&query, this](const auto& word) {
        const QueryWord query_word = this->ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(as_string(query_word.data));
            }
            else {
                query.plus_words.insert(as_string(query_word.data));
            }
        }
        return query;
        }
    );

}