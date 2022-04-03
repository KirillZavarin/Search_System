#include "document.h"


Document::Document()
    :id(0), relevance(0), rating(0)
{}

Document::Document(int id_, double relevance_, int rating_)
    : id(id_), relevance(relevance_), rating(rating_)
{}

std::ostream& operator<<(std::ostream& out, Document doc) {
    using namespace std::string_literals;
    out << '{' << " document_id = "s << doc.id << ", relevance = "s << doc.relevance << ", rating = "s << doc.rating << " }";
    return out;
}