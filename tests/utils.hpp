#ifndef TESTS_UTILS_HPP
#define TESTS_UTILS_HPP

#include <string>
#include <functional>

/**
 * @brief Checks if a function throws a std::runtime_error with a specific message.
 *
 * @param function The function to execute.
 * @param expectedMessage The expected error message.
 * @return true if func throws std::runtime_error with expectedMessage, else false.
 */
bool checkRuntimeError(std::function<void()> function, const std::string &expectedMessage);

#endif // TESTS_UTILS_HPP
