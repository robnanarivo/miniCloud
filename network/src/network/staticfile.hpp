#pragma once

#include "datachunk.hpp"

struct StaticFile {
    std::string filename;
    std::string type;
    DataChunk data;

    StaticFile(const std::string& file_path);
    StaticFile(const std::string& filename, const std::string& type, DataChunk data);

    static std::string parse_content_type(const std::string& path);
private:
    static const std::unordered_map<std::string, std::string> extension_mapping;
};
