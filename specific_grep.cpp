#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

/**
 * Searches for a given string in a vector of files and returns a vector of tuples that contain
 * the thread ID, file path, line number, and line that matches the search string.
 *
 * @param search_string The string to search for.
 * @param files_to_search A vector of file paths to search in.
 * @return A vector of tuples containing thread ID, file path, line number, and line that match the search string.
 */
std::vector<std::tuple<std::thread::id, std::string, int, std::string>> searchFilesForString(const std::string& search_string, const std::vector<fs::path>& files_to_search) {
	// Initialize the vector that will contain the search results
	std::vector<std::tuple<std::thread::id, std::string, int, std::string>> results;

	// Get the ID of the current thread
	std::thread::id thread_id = std::this_thread::get_id();

	// Loop through each file path in the vector and search for the string
	for (const auto& file_path : files_to_search) {
		// Open the file for reading
		std::ifstream file(file_path);

		// If the file was successfully opened, read each line and search for the string
		if (file.good()) {
			std::string line;
			int line_number = 0;
			while (std::getline(file, line)) {
				++line_number;
				if (line.find(search_string) != std::string::npos) {
					// If the string was found, add the search results to the vector
					results.emplace_back(thread_id, file_path.string(), line_number, line);
				}
			}
			file.close();
		}
		// If the file could not be opened, output an error message
		else {
			std::cerr << "Error: could not open file " << file_path.string() << " due to permission issues." << std::endl;
		}
	}

	// Return the vector of search results
	return results;
}


int main()
{
	return 0;
}