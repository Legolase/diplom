#include <utils/stream_reader.hpp>

utils::StreamReader::StreamReader(std::istream& in) noexcept :
    stream(in) {}