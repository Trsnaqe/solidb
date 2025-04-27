#pragma once

#include <string>
#include <vector>
#include <algorithm>

namespace soliddb {
namespace util {

/**
 * String utility functions
 */
class StringUtils {
public:
    /**
     * Tokenize a string into parts based on a delimiter
     */
    static std::vector<std::string> tokenize(const std::string& input, char delimiter);

    /**
     * Trim whitespace from beginning and end of string
     */
    static std::string trim(const std::string& str);

    /**
     * Convert string to uppercase
     */
    static std::string toUpper(const std::string& str);

    /**
     * Convert string to lowercase
     */
    static std::string toLower(const std::string& str);

    /**
     * Check if string starts with a prefix
     */
    static bool startsWith(const std::string& str, const std::string& prefix);

    /**
     * Check if string ends with a suffix
     */
    static bool endsWith(const std::string& str, const std::string& suffix);
};

} // namespace util
} // namespace soliddb 