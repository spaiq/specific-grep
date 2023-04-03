# Specific Grep - A C++ program for searching files for a specific pattern
## Program Description
Specific Grep is a command-line tool written in C++ that searches files for a specific pattern. It is similar to the popular Linux command "grep -r" but has additional features like multi-threading, logging, and customizable output files. Specific Grep can search in a specified directory and all its subdirectories. It produces a result file that lists all files where the pattern was found, along with the line number and line content, and a log file that shows the thread IDs and file names processed.
## Installation and usage
To use Specific Grep, you need to have a C++ compiler installed on your system. You can compile the program by running the following command in your terminal:
```
g++ specific_grep.cpp -o specific_grep -std=c++17 -lpthread
```
After compiling, you can run the program by typing the following command in your terminal:
```
./specific_grep <pattern> [-d <directory>] [-l <log_file>] [-r <result_file>] [-t <threads>]
```
Replace <pattern> with the pattern you want to search for. If you don't specify any other parameters, the program will use default values for directory, log file, result file, and threads.
### Parameters
Specific Grep has four optional parameters that you can use to customize the search:
-d or --dir: the directory where the program should start looking for files (including subdirectories). Default: current directory.
-l or --log_file: the name of the log file that the program should produce. Default: <program name>.log.
-r or --result_file: the name of the file where the program should write the search results. Default: <program name>.txt.
-t or --threads: the number of threads that the program should use for searching. Default: 4.
### Output Files
When Specific Grep finishes its work, it produces two output files:
The result file: <result_file> (default: <program name>.txt). It contains a list of all files where the pattern was found, along with the line number and line content. The list is sorted from the file where the most patterns were found to the one with the least.
The log file: <log_file> (default: <program name>.log). It contains a list of thread IDs and file names processed, sorted from the thread ID with the most files to the one with the least.
## License
Specific Grep is released under the MIT License. See LICENSE file for details.
