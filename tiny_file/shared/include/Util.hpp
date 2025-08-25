// Reference:
// https://github.com/tmwilliamlin168/CP-YouTube/blob/master/Google%20Kickstart/2022/Round%20B/A.cpp
// Template for loops

#pragma once

#define THROW_RUNTIME_ERROR(msg)                                                                   \
    throw std::runtime_error((msg) + std::string(" at ") + std::string(__FILE__) + ":" +           \
                             std::to_string(__LINE__))

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"