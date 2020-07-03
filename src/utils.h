#ifndef PCSR2_UTILS_H
#define PCSR2_UTILS_H

#include <sys/stat.h>

// https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c
bool endsWith (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

std::size_t fsize(const string& fname)
{
    struct stat st;
    if (0 == stat(fname.c_str(), &st)) {
        return st.st_size;
    }
    perror("stat issue");
    return -1L;
}

#endif //PCSR2_UTILS_H
