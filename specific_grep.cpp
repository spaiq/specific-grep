#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <thread>
#include <future>

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
	// Add entry for thread id storage if there was no file with the string in files subset
	if (results.empty()) {
		results.emplace_back(thread_id, "", NULL, "");
	}

	// Return the vector of search results
	return results;
}


/**
 * Search a directory and its subdirectories for files containing a given string.
 *
 * @param search_string The string to search for.
 * @param directory_path The path to the directory to search.
 * @param thread_count The number of threads to use for the search.
 * @return A pair containing a vector of tuples representing the search results and an integer representing the total number of files searched.
 */
std::pair<std::vector<std::tuple<std::thread::id, std::string, int, std::string>>, int> searchDirectoryForString(const std::string& search_string, const std::string& directory_path, const int thread_count) {
	// Create a vector of paths to all regular files in the directory and its subdirectories.
	std::vector<fs::path> files_to_search;
	for (const auto& file : fs::recursive_directory_iterator(directory_path)) {
		if (fs::is_regular_file(file)) {
			files_to_search.push_back(file.path());
		}
	}

	// Calculate the number of files to search per thread.
	const int files_per_thread = files_to_search.size() / thread_count;

	// Create a vector of futures representing the search results for each thread.
	std::vector<std::future<std::vector<std::tuple<std::thread::id, std::string, int, std::string>>>> futures;
	for (int i = 0; i < thread_count; ++i) {
		const int start_index = i * files_per_thread;
		const int end_index = (i + 1) == thread_count ? files_to_search.size() : (i + 1) * files_per_thread;
		const std::vector<fs::path> files_subset(files_to_search.begin() + start_index, files_to_search.begin() + end_index);

		// Launch a new thread to search the subset of files.
		std::future<std::vector<std::tuple<std::thread::id, std::string, int, std::string>>> result = std::async(std::launch::async, searchFilesForString, std::cref(search_string), files_subset);
		futures.emplace_back(std::move(result));
	}

	// Collect the results from each thread and combine them into a single vector.
	std::vector<std::tuple<std::thread::id, std::string, int, std::string>> results;
	for (auto& future : futures) {
		std::vector<std::tuple<std::thread::id, std::string, int, std::string>> result = future.get();
		results.insert(results.end(), result.begin(), result.end());
	}

	// Return a pair containing the search results and the total number of files searched.
	return std::make_pair(results, files_to_search.size());
}


int main()
{
	return 0;
}