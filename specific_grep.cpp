#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <thread>
#include <future>
#include <regex>
#include <set>
#include <map>

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


/**
 * Writes the results to a file in the specified format.
 * The results are a vector of tuples, each containing a thread ID, a file path, a line number,
 * and the content of the line.
 *
 * @param output_filename The name of the output file to write to.
 * @param results A vector of tuples containing the results to write to the file.
 */
void writeResultsToFile(const std::string& output_filename, const std::vector<std::tuple<std::thread::id, std::string, int, std::string>>& results) {
	// Map to store the file patterns (line numbers and content) for each file.
	std::map<std::string, std::vector<std::tuple<int, std::string>>> file_patterns_map;

	// Populate the map with file patterns.
	for (const auto& result : results) {
		// Extract the relevant values from the tuple.
		const auto& [thread_id, file_path, line_number, line_content] = result;

		// Check that the values are not empty or zero.
		if (!file_path.empty() && line_number && !line_content.empty()) {
			// Add the line number and content to the vector for this file path.
			file_patterns_map[file_path].push_back({ line_number, line_content });
		}
	}

	// Sort the patterns for each file by line number.
	for (auto& [file_path, patterns] : file_patterns_map) {
		std::sort(patterns.begin(), patterns.end(), [](const auto& lhs, const auto& rhs) {
			return std::get<0>(lhs) < std::get<0>(rhs);
			});
	}

	// Sort the files by number of patterns.
	std::vector<std::pair<std::string, std::vector<std::tuple<int, std::string>>>> sorted_files(file_patterns_map.begin(), file_patterns_map.end());
	std::sort(sorted_files.begin(), sorted_files.end(), [](const auto& lhs, const auto& rhs) {
		return lhs.second.size() > rhs.second.size();
		});

	// Write the sorted patterns to the output file.
	std::ofstream output_file(output_filename + ".txt");
	if (!output_file.is_open()) {
		std::cerr << "Could not open output file" << std::endl;
		return;
	}

	// Iterate over each file's patterns and write them to the output file.
	for (const auto& [file_path, patterns] : sorted_files) {
		for (const auto& [line_number, line_content] : patterns) {
			// Write the file path, line number, and content in the specified format.
			output_file << file_path << ":" << line_number << ": " << line_content << std::endl;
		}
	}

	output_file.close();
}


/**
 * Writes log information to a file.
 *
 * @param filename The name of the file to write to.
 * @param results A vector of tuples containing log information for each thread.
 */
void writeLogToFile(const std::string& filename, const std::vector<std::tuple<std::thread::id, std::string, int, std::string>>& results) {
	// Open the output file for writing.
	std::ofstream output_file(filename + ".log");
	if (!output_file.is_open()) {
		std::cerr << "Unable to open file for writing: " << filename << std::endl;
		return;
	}

	// Create a map to store the files associated with each thread.
	std::map<std::thread::id, std::vector<std::string>> threads_to_files;

	// Populate the map with file names for each thread.
	for (const auto& result : results) {
		std::thread::id thread_id = std::get<0>(result);
		std::string file_name = std::get<1>(result);
		threads_to_files[thread_id].push_back(file_name);
	}

	// Convert the map to a vector of pairs.
	std::vector<std::pair<std::thread::id, std::vector<std::string>>> thread_file_pairs;
	std::copy(threads_to_files.begin(), threads_to_files.end(), std::back_inserter(thread_file_pairs));

	// Sort the vector of thread-file pairs by the number of files associated with each thread.
	std::sort(thread_file_pairs.begin(), thread_file_pairs.end(), [](const auto& lhs, const auto& rhs) {
		if (lhs.second[0] == "" && rhs.second[0] != "") {
			return false;
		}
		else if (lhs.second[0] != "" && rhs.second[0] == "") {
			return true;
		}
		else {
			return lhs.second.size() > rhs.second.size();
		}
		});

	// Write the sorted thread-file pairs to the output file.
	for (const auto& [thread_id, file_names] : thread_file_pairs) {
		output_file << thread_id << ":";
		for (const auto& file_name : file_names) {
			output_file << file_name << ",";
		}
		// Remove the trailing comma and add a newline character.
		output_file.seekp(-1, std::ios_base::end);
		output_file << std::endl;
	}

	// Close the output file.
	output_file.close();
}



/**
* Print the search results to the console, including the number of searched files,
* the number of files containing the search pattern, the number of unique pattern occurrences,
* the name of the result file, the name of the log file, the number of threads used in the search,
* and the elapsed time.
*
* @param results A pair consisting of the search results and the number of searched files.
* @param thread_count The number of threads used in the search.
* @param log_filename The name of the log file to be generated.
* @param result_filename The name of the result file to be generated.
* @param elapsed_time_ms The elapsed time in milliseconds.
*/
void printSearchResults(const std::pair<std::vector<std::tuple<std::thread::id, std::string, int, std::string>>, int>& results, int thread_count, std::string log_filename, std::string result_filename, int& timer_start) {
	// Extract search results.
	std::vector<std::tuple<std::thread::id, std::string, int, std::string>> results_vector = results.first;

	// Print number of searched files.
	std::cout << "Searched files: " << results.second << std::endl;

	// Count files with pattern and pattern occurrences.
	std::set<std::string> files_with_pattern;
	std::set<std::tuple<std::string, int>> pattern_occurrences;
	for (const auto& result : results_vector) {
		if (std::get<2>(result) != 0) {
			files_with_pattern.insert(std::get<1>(result));
			pattern_occurrences.insert(std::make_tuple(std::get<1>(result), std::get<2>(result)));
		}
	}

	// Print number of files with pattern and number of unique pattern occurrences.
	std::cout << "Files with pattern: " << files_with_pattern.size() << std::endl;
	std::cout << "Patterns number: " << pattern_occurrences.size() << std::endl;

	// Print name of result file and log file, number of threads used, and elapsed time.
	std::cout << "Result file: " << result_filename << ".txt" << std::endl;
	std::cout << "Log file: " << log_filename << ".log" << std::endl;
	std::cout << "Used threads: " << thread_count << std::endl;

	// Stop the timer and calculate the elapsed time of the program
	int timer_stop = clock();
	double elapsed_time_ms = (timer_stop - timer_start) / (double)CLOCKS_PER_SEC * 1000;
	std::cout << "Elapsed time: " << elapsed_time_ms << "[ms]" << std::endl;
}


/**
 * Determines if a given filename is valid, meaning it contains only alphanumeric
 * characters, hyphens, dots, and spaces.
 *
 * @param filename the filename to validate
 * @return true if the filename is valid, false otherwise
 */
bool isValidFilename(const std::string& filename) {
	// Define a regular expression pattern to match characters that are not alphanumeric,
	// hyphens, dots, or spaces.
	static const std::regex pattern("[^\\w\\-. ]");

	// Use regex_search to check if the filename contains any invalid characters.
	// If the search returns true, it means the filename is invalid, so return false.
	// If the search returns false, it means the filename is valid, so return true.
	return !std::regex_search(filename, pattern);
}


int main(int argc, char* argv[]) {
	// Initialize the timer
	int timer_start;

	// Start the timer
	timer_start = clock();

	// Extract the filename from the first argument
	std::string filename = std::filesystem::path(argv[0]).filename().string();

	// If no arguments are given or the number of arguments is invalid, print an error message and exit
	if (argc == 1) {
		std::cerr << "Error: wrong usage of the program\n"
			<< "Usage: " << filename << " <search string> [options]\n"
			<< "Options:\n"
			<< "  -d <directory> - directory to search in (default: current directory)\n"
			<< "  -l <log filename> - log filename (default: <program name>.log)\n"
			<< "  -r <result filename> - result filename (default: <program name>.txt)\n"
			<< "  -t <thread count> - number of threads to use (default: 4)\n";
		return 1;
	}

	if (!(argc % 2 == 0) || argc > 10) {
		std::cerr << "Error: wrong number of arguments" << std::endl;
		return 1;
	}

	// Set default values for directory path, log filename, result filename, and thread count
	std::string directory_path = std::filesystem::current_path().string();
	std::string search_string = argv[1];
	int additional_options_cnt = (argc - 2) / 2, thread_cnt = 4;
	bool dir_opt = false, log_filename_opt = false, result_filename_opt = false, thread_cnt_opt = false;

	// Extract the program name from the filename
	std::size_t last_dot = filename.find_last_of(".");
	std::string program_name = filename.substr(0, last_dot);
	std::string log_filename = program_name;
	std::string result_filename = program_name;

	// Loop through the additional options
	for (int i = 1; i <= additional_options_cnt; i++) {
		// If the option is the -d or --dir option, set the directory path
		if (strcmp(argv[i * 2], "-d") == 0 || strcmp(argv[i * 2], "--dir") == 0) {
			// Check if the directory option has already been set
			if (dir_opt == true) {
				std::cerr << "Error: multiple usage of the starting directory option" << std::endl;
				return 1;
			}
			// Append the directory to the current directory path
			directory_path = directory_path + "\\" + argv[i * 2 + 1];
			// Check if the directory exists
			if (!fs::exists(directory_path)) {
				if (!fs::exists(argv[i * 2 + 1])) {
					std::cerr << "Error: directory does not exist" << std::endl;
					return 1;
				}
				directory_path = argv[i * 2 + 1];
			}
			// Set the directory option to true
			dir_opt = true;
		}
		// If the option is the -l or --log_file option, set the log filename
		else if (strcmp(argv[i * 2], "-l") == 0 || strcmp(argv[i * 2], "--log_file") == 0) {
			// Check if option already used
			if (log_filename_opt == true) {
				std::cerr << "Error: multiple usage of the log filename option" << std::endl;
				return 1;
			}
			// Set log filename and check if valid
			log_filename = argv[i * 2 + 1];
			if (!isValidFilename(log_filename)) {
				std::cerr << "Error: invalid log filename" << std::endl;
				return 1;
			}
			log_filename_opt = true;
		}
		// If the option is the -r or --result_file option, set the result filename
		else if (strcmp(argv[i * 2], "-r") == 0 || strcmp(argv[i * 2], "--result_file") == 0) {
			// Check if option already used
			if (result_filename_opt == true) {
				std::cerr << "Error: multiple usage of the result filename option" << std::endl;
				return 1;
			}
			// Set result filename and check if valid
			result_filename = argv[i * 2 + 1];
			if (!isValidFilename(result_filename)) {
				std::cerr << "Error: invalid result filename" << std::endl;
				return 1;
			}
			result_filename_opt = true;
		}
		// If the option is the -t or --threads option, set the threads count
		else if (strcmp(argv[i * 2], "-t") == 0 || strcmp(argv[i * 2], "--threads") == 0) {
			// Check if option already used
			if (thread_cnt_opt == true) {
				std::cerr << "Error: multiple usage of the thread count option" << std::endl;
				return 1;
			}
			// Set thread count and catch invalid argument
			try {
				thread_cnt = std::stoi(argv[i * 2 + 1]);
			}
			catch (const std::invalid_argument& e) {
				std::cerr << "Error: invalid thread count" << std::endl;
				return 1;
			}
			thread_cnt_opt = true;
		}
		// If option not recognized, print error message
		else {
			std::cerr << "Wrong usage of the additional parameters." << std::endl;
			return 1;
		}
	}

	// Search directory for string with specified thread count
	std::pair<std::vector<std::tuple<std::thread::id, std::string, int, std::string>>, int> results = searchDirectoryForString(search_string, directory_path, thread_cnt);

	// Write results to the file specified by result_filename variable
	writeResultsToFile(result_filename, std::get<0>(results));

	// Write the log file to the file specified by log_filename variable
	writeLogToFile(log_filename, std::get<0>(results));

	// Print the results of the program
	printSearchResults(results, thread_cnt, log_filename, result_filename, timer_start);

	// Return success
	return 0;
}
