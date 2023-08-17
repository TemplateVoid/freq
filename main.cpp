#include <iostream>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <fstream>

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cctype>

namespace freq {
namespace detail {

// Read file and add words to unordered_map (use mmap)
std::unordered_map<std::string, size_t> frequencyDict(const char *filename)
{
    // open file
    int fd = ::open(filename, O_RDONLY);
    if (fd == 1) {
        throw std::runtime_error("Failed to open file!");
    }

    // get file size
    struct stat info;
    int result = fstat(fd, &info);
    if (result == -1) {
        close(fd);
        throw std::runtime_error("Can't get file info!");
    }

    // mmap file
    char *addr = reinterpret_cast<char*>(::mmap(nullptr, info.st_size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0));
    if (addr == MAP_FAILED) {
        close(fd);
        throw std::runtime_error("Failed to mmap!");
    }

    auto data = addr;

    // set hints that we have sequential access
    result = madvise(data, info.st_size, MADV_SEQUENTIAL | MADV_WILLNEED);
    if (result != 0) {
        std::cerr << "Failed to set hints!\n";
    }

    // read like from memory buffer
    std::string word;
    std::unordered_map<std::string, size_t> dict;
    for (int i = 0; i < info.st_size; ++i) {
        if (::isalpha(data[i])) {
            word += ::tolower(data[i]);
        }
        else if (!word.empty()) {
            ++dict[word];
            word.clear();
        }
    }
    if (!word.empty())
        ++dict[word];

    // unmap
    ::munmap(addr, info.st_size);
    // close file
    ::close(fd);

    return dict;

}
}

void freq(const char *input, const char *output)
{
    const auto dict = detail::frequencyDict(input);

    std::vector<decltype(dict.begin())> iterators;
    iterators.reserve(dict.size());
    for (auto it = dict.begin(); it != dict.end(); ++it) {
        iterators.push_back(it);
    }
    // sort
    std::sort(iterators.begin(), iterators.end(), [](auto it1, auto it2) {
        return it1->second == it2->second ? it1->first < it2->first : it1->second > it2->second;
    });

    // write to output file
    std::ofstream file(output);
    for (auto it: iterators) {
        file << it->first << " " << it->second << "\n";
    }
}

}

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cout << "Wrong number of input parameters!\n" << "Usage: ./freq <input> <output>\n";
        return -1;
    }

    try {
        freq::freq(argv[1], argv[2]);
    }
    catch (std::exception &ex) {
        std::cerr << ex.what();
        return -1;
    }
    return 0;
}
