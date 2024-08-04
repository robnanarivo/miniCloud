#pragma once

#include <memory>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct DataChunk {
    // the actual data
    std::unique_ptr<unsigned char[]> data;

    // size of the data array
    size_t size;

    // Ctors
    DataChunk();
    DataChunk(size_t size);
    DataChunk(const std::string& str);
    DataChunk(const std::vector<unsigned char>& vec);

    DataChunk(const DataChunk& other);

    DataChunk(DataChunk&& other);

    DataChunk& operator=(const DataChunk& other);

    DataChunk& operator=(DataChunk&& other);

    // convert to unsigned char vector
    std::vector<unsigned char> to_vec() const;

    // convert to std string
    std::string to_str() const;

    // convert to json
    json to_json() const;
};
