// MIT License
//
// Copyright (c) 2026 Leick Robinson (Virtual Science Workshop)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#ifndef __STRING_UTILS_HPP__
#define __STRING_UTILS_HPP__

#include <string>
#include <vector>

namespace string_utils {


// Split a string into a vector of substrings, based on a delimiter
std::vector<std::string> &splitString(const std::string &s, char delim,
                                                  std::vector<std::string> &res_vec)
{
    std::stringstream ss(s);
    std::string sval;
    while (std::getline(ss, sval, delim)) {
        res_vec.push_back(sval);
    }
    return res_vec;
}


// Split a string into two strings, based on the first occurrence of a delimiter.
// The "left" string will be replaced with the substring before the first occurrence of the
// delimiter, and the remainder of the original string after the delimiter will be placed in
// the "right" string.
// A reference to the first ("left") substring will also be returned (as a convenience).
// If the delimiter does not occur in the original string, the "right" string will be empty.
std::string &splitStringOnFirst(const std::string &s, char delim,
                                          std::string &left, std::string &right)
{
    std::stringstream ss(s);
    std::string sval;

    std::getline(ss, left, delim);
    ss >> right;

    return left;
}



// Match two strings, where the first may contain wildcards, with the following rules:
// ? will match any one character
// * will match zero or more characters
bool wildMatch(const char *wild, const char *normal) {
    // If we've reached the end of both, success!
    if ((*wild == '\0') && (*normal == '\0')) { return true; }

    // If the wild string contains '*', then either we can skip the '*' and the rest of both
    // strings match, or we skip the next character in the normal string and try again.
    // (we need to do this here, before the comparison below, just in case the normal string
    // happens to contain a '*' character.)
    if (*wild == '*') {
        // But, first need to check to see if there are any characters left in the normal string
        if (*normal == '\0') { return wildMatch(wild+1, normal); }
        else                 { return (wildMatch(wild+1, normal) || wildMatch(wild, normal+1)); }
    }

    // If the characters match, or the wild string contains "?", continue
    if ((*wild == '?') || (*wild == *normal)) { return wildMatch(wild+1, normal+1); }

    // The strings don't match
    return false;
}

bool wildMatch(const std::string &wild_str, const std::string &normal_str) {
    return wildMatch(wild_str.c_str(), normal_str.c_str());
}

}; // namespace string_utils


#endif // __STRING_UTILS_HPP__
