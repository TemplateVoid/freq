#include <iostream>
#include <unordered_map>
#include <memory>
#include <string>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <random>
#include <chrono>

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cctype>

static constexpr size_t AlphabetSize = 26;

// naive trie implementation
struct TrieNode
{
    TrieNode() {
        std::fill_n(children, AlphabetSize, nullptr);
    }

    ~TrieNode() noexcept {
        for (auto child: children)
            delete child;
    }

    void insert(const std::string &key)
    {
        TrieNode *node = this;
        for (auto ch: key) {
            const int index = ch - 'a';
            if (node->children[index] == nullptr)
                node->children[index] = new TrieNode;

            node = node->children[index];
        }
        node->isWord = true;
        ++count;
    }

private:
    TrieNode *children[AlphabetSize];
    size_t count = 0;
    bool isWord = false;

};

std::unordered_map<std::string, size_t> frequencyDictMmap(const char *filename)
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

std::unordered_map<std::string, size_t> frequencyDictStream(const char *filename)
{
    std::unordered_map<std::string, size_t> dict;
    std::ifstream file(filename);
    // just read word by word
    for (std::string word; file >> word;) {
        std::transform(word.cbegin(), word.cend(), word.begin(), ::tolower);
        ++dict[word];
    }
    return dict;
}

std::unique_ptr<TrieNode> frequencyDictStreamTrie(const char *filename)
{
    auto dict = std::make_unique<TrieNode>();
    std::ifstream file(filename);
    for (std::string word; file >> word;) {
        std::transform(word.cbegin(), word.cend(), word.begin(), ::tolower);
        dict->insert(word);
    }
    return dict;
}


namespace test {

struct Timer {
    Timer(const std::string &name): point(std::chrono::steady_clock::now()), name(name) {}
    ~Timer() {
        const std::chrono::duration<double> seconds = std::chrono::steady_clock::now() - point;
        std::cout << name << " " << seconds.count() << "\n";
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> point;
    std::string name;
};

void generateTextFile(const std::string &filename, size_t size, size_t minWordSize = 5, size_t maxWordSize = 5) {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> distribLetter(0, 25);
    std::uniform_int_distribution<> distribWordSize(minWordSize, maxWordSize);

    std::string text;
    text.reserve(size);

    while (size > 0) {
        size_t wordSize = distribWordSize(gen);
        if (wordSize > size) {
            wordSize = size;
        }
        size -= wordSize;
        for (; wordSize > 0; --wordSize) {
            text.push_back(distribLetter(gen) % 2 == 0 ? distribLetter(gen) + 65 : distribLetter(gen) + 97);
        }
        if (size > 0) {
            text.push_back(' ');
            --size;
        }
    }

    std::ofstream file(filename.c_str());
    file << text;
}

void benchmark(size_t filesize, size_t minWordSize, size_t maxWordSize) {

    std::cout << "Benchmark: " << filesize << ", " << minWordSize << ", " << maxWordSize << "\n";

    generateTextFile("test.txt", filesize, minWordSize, maxWordSize);

    {
        Timer timer("Mmap read");
        auto dict = frequencyDictMmap("test.txt");
    }

    {
        Timer timer("Stream read");
        auto dict = frequencyDictStream("test.txt");
    }

    {
        Timer timer("Stream read trie");
        auto dict = frequencyDictStreamTrie("test.txt");
    }
}

}

int main()
{
    test::benchmark(1024 * 1024, 1, 10);
    test::benchmark(1024 * 1024, 5, 5);
    test::benchmark(10 * 1024 * 1024, 3, 10);
    test::benchmark(100 * 1024 * 1024, 5, 10);
    return 0;
}
