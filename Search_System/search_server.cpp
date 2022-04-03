#include "search_server.h"

using namespace std::string_literals;

SearchServer::SearchServer(const std::string_view text) {
    for (const std::string_view& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Invalid symbol in "s + as_string(word) + " word"s);
        };
        stop_words_.insert(as_string(word));
    }
}

SearchServer::SearchServer(const std::string text) : SearchServer(std::string_view(text)) {}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Document id < 0"s);
    }
    if (documents_.count(document_id)) {
        throw std::invalid_argument("Document with this document id, is in the list"s);
    }
    if (!IsValidWord(document)) {
        throw std::invalid_argument("Invalid symbol in " + document_id);
    };
    const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();

    for (const std::string_view& word_view : words) {
        std::string word = as_string(word_view);
        word_to_document_freqs_[word][document_id] += inv_word_count;
        id_word_to_freqs[document_id][word] += inv_word_count;
    }

    documents_.emplace(document_id,
        DocumentData{
            ComputeAverageRating(ratings),
            status
        });

    IDs.insert(document_id);

}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status_) const {
    return FindTopDocuments(raw_query, [status_](int document_id, DocumentStatus status, int rating) { return status == status_; });
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query, int document_id) const {

    if (IDs.count(document_id) == 0) {
        throw std::out_of_range("Not found document id");
    }

    if (!IsValidWord(raw_query)) {
        throw std::invalid_argument("Invalid symbol in " + document_id);
    };

    const Query query = ParseQuery(raw_query);

    std::vector<std::string_view> matched_words;

    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.at(as_string(word)).count(document_id)) {
            return { matched_words, documents_.at(document_id).status };
        }
        if (word_to_document_freqs_.count(as_string(word)) == 0) {
            continue;
        }
    }

    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(as_string(word)) == 0) {
            continue;
        }
        char* arr = new char[word.size() + 1];

        for (size_t i = 0; i < word.size(); i++) {
            arr[i] = word.at(i);
        }

        std::string_view push_view(arr, word.size());

        if (word_to_document_freqs_.at(as_string(word)).count(document_id)) {
            matched_words.push_back(push_view);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy par, const std::string_view& raw_query, int document_id) const {

    if (IDs.count(document_id) == 0) {
        throw std::out_of_range("Not found document id");
    }

    if (!IsValidWord(raw_query)) {
        throw std::invalid_argument("Invalid symbol in " + document_id);
    }

    const Query query = ParseQuery(par, raw_query);

    std::vector<std::string_view> matched_words;

    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.at(as_string(word)).count(document_id)) {
            return { matched_words, documents_.at(document_id).status };
        }
        if (word_to_document_freqs_.count(as_string(word)) == 0) {
            continue;
        }
    }

    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(as_string(word)) == 0) {
            continue;
        }
        char* arr = new char[word.size() + 1];

        for (size_t i = 0; i < word.size(); i++) {
            arr[i] = word.at(i);
        }

        std::string_view push_view(arr, word.size());

        if (word_to_document_freqs_.at(as_string(word)).count(document_id)) {
            matched_words.push_back(push_view);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy seq, const std::string_view& raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

bool SearchServer::IsStopWord(const std::string_view& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
    std::vector<std::string_view> words;
    for (const std::string_view& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;

    if (!IsValidWord(text)) {
        throw std::invalid_argument(" Invalid symbol in "s + as_string(text) + " word"s);
    };

    if (text.empty()) {
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }

    if (text == "-") {
        throw std::invalid_argument("no word after minus"s);
    }

    if (text[0] == '-' && text.size() > 1) {
        if (text[1] == '-') {
            throw std::invalid_argument("double minus in the word"s);
        }
        is_minus = true;
        text = text.substr(1);
    }

    return {
        text,
        is_minus,
        IsStopWord(text)
    };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const {
    Query query;
    for (const std::string_view word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(as_string(query_word.data));
            }
            else {
                query.plus_words.insert(as_string(query_word.data));
            }
        }
    }

    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsValidWord(const std::string& word) const {
    for (const char c : word) {
        if (c >= '\0' && c < ' ') {
            return false;
        }
    }
    return true;
}

bool SearchServer::IsValidWord(const std::string_view word) const {
    for (const char c : word) {
        if (c >= '\0' && c < ' ') {
            return false;
        }
    }
    return true;
}

std::set<int>::iterator SearchServer::begin() const {
    return IDs.begin();
}

std::set<int>::iterator SearchServer::end() const {
    return IDs.end();
}

void SearchServer::RemoveDocument(int document_id) {

    const auto found = documents_.find(document_id);
    if (found == documents_.end()) {
        return;
    }

    IDs.erase(document_id);

    std::map<int, DocumentData>::iterator element_to_delet = documents_.find(document_id);
    documents_.erase(element_to_delet);

    std::vector<std::string> words_to_del;

    for (auto& [word, freqs] : id_word_to_freqs.at(document_id)) {
        word_to_document_freqs_.at(as_string(word)).erase(document_id);
        if (word_to_document_freqs_.at(as_string(word)).empty()) {
            word_to_document_freqs_.erase(as_string(word));
        }
    }

    id_word_to_freqs.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy seq, int document_id){
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy par, int document_id) {
    const auto found = documents_.find(document_id);
    if (found == documents_.end()) {
        return;
    }

    IDs.erase(document_id);

    std::map<int, DocumentData>::iterator element_to_delet = documents_.find(document_id);
    documents_.erase(element_to_delet);

    std::vector<std::string> words_to_del;

    std::for_each(par, id_word_to_freqs.at(document_id).begin(), id_word_to_freqs.at(document_id).end(), [&](std::pair<const std::string_view, double>& pair_) {
        word_to_document_freqs_.at(as_string(pair_.first)).erase(document_id);
        if (word_to_document_freqs_.at(as_string(pair_.first)).empty()) {
            word_to_document_freqs_.erase(as_string(pair_.first));
        }
    });

    id_word_to_freqs.erase(document_id);
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    return id_word_to_freqs.at(document_id);
}