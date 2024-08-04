#include "datachunk.hpp"
#include <string.h>

DataChunk::DataChunk()
    : data(nullptr),
      size(0) {};

DataChunk::DataChunk(size_t size)
    : data(std::make_unique<unsigned char[]>(size)),
      size(size) {}

DataChunk::DataChunk(const std::string& str)
    : data(std::make_unique<unsigned char[]>(str.size())),
      size(str.size()) {
    memcpy(data.get(), str.data(), str.size());
}

DataChunk::DataChunk(const std::vector<unsigned char>& vec)
    : data(std::make_unique<unsigned char[]>(vec.size())),
      size(vec.size()) {
    memcpy(data.get(), vec.data(), vec.size());
}

DataChunk::DataChunk(const DataChunk& other)
    : data(std::make_unique<unsigned char[]>(other.size)),
      size(other.size) {
    memcpy(data.get(), other.data.get(), other.size);
}

DataChunk::DataChunk(DataChunk&& other)
    : data(std::move(other.data)),
      size(other.size) {}

DataChunk& DataChunk::operator=(const DataChunk& other) {
    data.reset();
    size = other.size;
    data = std::make_unique<unsigned char[]>(size);
    memcpy(data.get(), other.data.get(), size);
    return *this;
}

DataChunk& DataChunk::operator=(DataChunk&& other) {
    data.swap(other.data);
    std::swap(size, other.size);
    return *this;
}

std::vector<unsigned char> DataChunk::to_vec() const {
    return std::vector<unsigned char>(data.get(), data.get() + size);
}

std::string DataChunk::to_str() const {
    std::string res;
    for (size_t i = 0; i < size; i++) {
        res += data[i];
    }
    return res;
}

json DataChunk::to_json() const {
    try {
       return json::parse(to_str());
    } catch (std::exception& e) {
        throw new std::runtime_error("DataChunk: unable to convert to json");
    }
}