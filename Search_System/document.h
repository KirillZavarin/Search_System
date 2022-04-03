#pragma once

#include <iostream>
#include <string>

struct Document {
    Document();

    Document(int id_, double relevance_, int rating_);

    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

std::ostream& operator<<(std::ostream& out, Document doc);