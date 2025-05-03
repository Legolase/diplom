#include <utils/common.hpp>

utils::BadStream::BadStream(const std::string& _what) :
    std::runtime_error(_what)
{
}