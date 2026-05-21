#include "utils.hpp"
#include <stdexcept>

bool checkRuntimeError(std::function<void()> function, const std::string& expectedMessage) {
    try {
        function();
    } catch (const std::runtime_error& e) {
        return std::string(e.what()) == expectedMessage;
    } catch (...) {
        return false;
    }
    return false;
}
