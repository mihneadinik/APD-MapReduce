#include <iostream>
#include <fstream>
#include <pthread.h>
#include <math.h>
#include <algorithm>
#include <vector>
#include <queue>
#include <unordered_map>
#include <forward_list>
#include <unordered_set>

// store {file_name, file_status} pairs in a priority queue ordered by
// the status; unopened files (status 0) will always be first, while
// opened and finished files (status 1/2) will be at the end
auto compare = [](std::pair<std::string, int> &f1, std::pair<std::string, int> &f2) {
    return f1.second > f2.second;
};

struct threads_info {
    int current_thread_id;
    int nr_mappers;
    bool *mappers_finished;
    int max_pow;
    pthread_mutex_t *mutex;
    pthread_barrier_t *barrier;
    std::unordered_map<int, std::vector<std::forward_list<int>>> *mappers_to_lists;
    std::unordered_map<int, std::vector<int>> *perfect_powers;;
    std::priority_queue<std::pair<std::string, int>,
                        std::vector<std::pair<std::string, int>>,
                        decltype(compare)> *files;
};

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
// initialise the map structure for mappers
// precompute perfect powers up to the maximum exponent
void init_values(int& max_pow, int& nr_threads,
                 std::unordered_map<int, std::vector<std::forward_list<int>>>& mappers_to_lists,
                 std::unordered_map<int, std::vector<int>>& perfect_powers,
                 int nr_reducers, int nr_mappers) {
    nr_threads = nr_reducers + nr_mappers;
    for (int i = 0; i < nr_mappers; i++) {
        std::vector<std::forward_list<int>> empty(nr_reducers);
        mappers_to_lists.insert({i, empty});
    }
    max_pow = ++nr_reducers;

    // precompute perfect powers
    for (int i = 2; i <= max_pow; i++) {
        for (int base = 2; pow(base, i) < INT32_MAX; base++) {
            perfect_powers[i].push_back(pow(base, i));
        }
    }
}

// parse input file and store the files to be used
void read_input_file(std::priority_queue<std::pair<std::string, int>,
                     std::vector<std::pair<std::string, int>>,
                     decltype(compare)>& files,
                     int& nr_files, std::string input_file) {
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
        files.push({line, 0});
    }

    file.close();
}

// actions performed by mappers only
void mappers_function(void *arg) {
    threads_info *thread_info = (threads_info *)arg;

    while (! *thread_info->mappers_finished) {
        pthread_mutex_lock(thread_info->mutex);

        // Only one thread at a time is allowed to check for an unopened file
        std::pair<std::string, int> top_file = thread_info->files->top();

        if (top_file.second == 0) {
            thread_info->files->pop();
            thread_info->files->push({top_file.first, 1});
        } else {
            *thread_info->mappers_finished = true;
            pthread_mutex_unlock(thread_info->mutex);
            break;
        }
        
        pthread_mutex_unlock(thread_info->mutex);

        // Exit the exclusion zone and start working on the file
        std::ifstream file (top_file.first);
        int nr_lines;

        if (file.is_open()) {
            std::string line;
            std::getline(file, line);
            nr_lines = stoi(line);
        } else {
            std::cout << "Error opening file: " << top_file.first << std::endl;
            exit(1);
        }

        while (file && nr_lines) {
            --nr_lines;
            int number;
            std::string line;
            std::getline(file, line);
            number = stoi(line);

            // Check if the read number is a perfect power
            for (int p = 2; p <= thread_info->max_pow; p++) {
                if (number == 1 ||
                    std::binary_search((*thread_info->perfect_powers)[p].begin(),
                              (*thread_info->perfect_powers)[p].end(), number)) {
                    (*thread_info->mappers_to_lists)[thread_info->current_thread_id].at(p - 2).push_front(number);
                }
            }
        }

        file.close();
    }
}

// actions performed by reducers only
void reducers_function(void *arg) {
    threads_info *thread_info = (threads_info *)arg;
    int list_index = thread_info->current_thread_id - thread_info->nr_mappers;
    std::string output_name = "out" + std::to_string(list_index + 2) + ".txt";
    std::unordered_set<int> unique;

    // save perfect powers in a set to exclude duplicates
    for (int i = 0; i < thread_info->nr_mappers; i++) {
        std::forward_list<int> list = (*thread_info->mappers_to_lists)[i].at(list_index);
        for (auto it = list.begin(); it != list.end(); it++) {
            unique.insert(*it);
        }
    }

    std::ofstream file (output_name);
    file << unique.size();
    file.close();
}

// control function for threads
void *driver_function(void *arg) {
    threads_info *thread_info = (threads_info *)arg;
    if (thread_info->current_thread_id < thread_info->nr_mappers) {
        mappers_function(arg);
    }

    pthread_barrier_wait(thread_info->barrier);

    if (thread_info->current_thread_id >= thread_info->nr_mappers) {
        reducers_function(arg);
    }

    pthread_exit(NULL);
}

// mappers from 0 to < nr_mappers
// reducers from nr_mappers to < nr_threads
int main(int argc, char *argv[]) {
    int nr_mappers, nr_reducers, max_pow, nr_files, nr_threads;
    bool mappers_finished = false;
    std::string input_file;
    std::unordered_map<int, std::vector<std::forward_list<int>>> mappers_to_lists;
    std::unordered_map<int, std::vector<int>> perfect_powers;
    std::priority_queue<std::pair<std::string, int>,
                        std::vector<std::pair<std::string, int>>,
                        decltype(compare)> files(compare);

    parse_arguments(argc, argv, nr_mappers, nr_reducers, input_file);
    read_input_file(files, nr_files, input_file);
    init_values(max_pow, nr_threads, mappers_to_lists, perfect_powers, nr_reducers, nr_mappers);

    pthread_t t_id[nr_threads];
    threads_info t_info[nr_threads];
    pthread_mutex_t mutex;
    pthread_barrier_t barrier;

    // create the threads and initialise mutex & barrier
    pthread_mutex_init(&mutex, NULL);
    pthread_barrier_init(&barrier, NULL, nr_threads);

    for (int i = 0; i < nr_threads; i++) {
		t_info[i] = threads_info {i, nr_mappers, &mappers_finished,max_pow,
                            &mutex, &barrier, &mappers_to_lists, &perfect_powers, &files};
        pthread_create(&t_id[i], NULL, driver_function, &t_info[i]);
	}

    // wait for the threads to finish and delete mutex & barrier
    for (int i = 0; i < nr_threads; i++) {
		pthread_join(t_id[i], NULL);
	}

    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);
    return 0;
}