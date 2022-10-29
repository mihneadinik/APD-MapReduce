#include <iostream>
#include <fstream>
#include <pthread.h>
#include <vector>

// read and convert command line arguments
void parse_arguments(int argc, char **argv, int& nr_mappers,
                        int& nr_reducers, std::string& input_file)
{
	if(argc != 4) {
		std::cout << "Usage: ./tema1 mappers reducers input_file\n";
		exit(1);
	}

	nr_mappers = atoi(argv[1]);
	nr_reducers = atoi(argv[2]);
    input_file = argv[3];
}

// find the maximum exponent to be checked and total number of threads
void init_values(int& max_exp, int& nr_threads,
                    int nr_reducers, int nr_mappers) {
    nr_threads = nr_reducers + nr_mappers;
    max_exp = ++nr_reducers;
}

// parse input file and store the files to be used
void read_input_file(std::vector<std::string>& files, int& nr_files,
                        std::string input_file) {
    std::ifstream file (input_file);
    int index = 0;

    if (file.is_open()) {
        std::string line;
        std::getline(file, line);
        nr_files = stoi(line);
    } else {
        std::cout << "Error opening input file\n";
		exit(1);
    }

    while (file && index < nr_files) {
        ++index;
        std::string line;
        std::getline(file, line);
        files.push_back(line);
    }

    file.close();
}

void print_string_vec(std::vector<std::string>& files) {
    for (std::string elem : files) {
        std::cout << elem << std::endl;
    }
}

// mappers from 0 to < nr_mappers
// reducers from nr_mappers to < nr_threads
int main(int argc, char *argv[]) {
    int nr_mappers, nr_reducers, max_exp, nr_files, nr_threads;
    std::string input_file;
    std::vector<std::string> files;

    parse_arguments(argc, argv, nr_mappers, nr_reducers, input_file);
    read_input_file(files, nr_files, input_file);
    init_values(max_exp, nr_threads, nr_reducers, nr_mappers);

    return 0;
}