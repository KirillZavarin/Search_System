#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
	std::vector<int> id_to_delete;
	std::set<std::set<std::string>> set_words;

	for (auto id : search_server) {
		std::set<std::string> words;

		for (const auto& [word, freqs] : search_server.GetWordFrequencies(id)) {
			words.insert(as_string(word));
		}

		if (set_words.count(words)) {
			id_to_delete.push_back(id);
		}
		else {
			set_words.insert(words);
		}
	}

	for (auto id : id_to_delete) {
		std::cout << "Found duplicate document id " << id << std::endl;
		search_server.RemoveDocument(id);
	}
}