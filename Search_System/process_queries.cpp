#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());

    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](auto querie) {return search_server.FindTopDocuments(querie); });
    return result;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> vec_vec_doc(queries.size());
    std::vector<Document> documents;


    std::transform(std::execution::par, queries.begin(), queries.end(), vec_vec_doc.begin(), [&search_server](auto querie) {return search_server.FindTopDocuments(querie); });

    auto merge_function = [&vec_vec_doc](auto&& vec1, auto&& vec2) -> std::vector<Document>
    {
        vec1.resize(vec1.size() + vec2.size());
        std::move(vec2.begin(), vec2.end(), vec1.end() - vec2.size());
        return std::move(vec1);
    };

    documents = std::reduce(std::execution::par, vec_vec_doc.begin(), vec_vec_doc.end(), std::vector<Document>{}, merge_function);

    return documents;
}