#pragma once

#include "network/datachunk.hpp"
#include "network/staticfile.hpp"

class StaticService {
private:
    const std::string path_prefix_;
public:
    // Ctor path_prefix must include trailing slash!
    StaticService(const std::string& path_prefix);
    StaticFile read_file(const std::string& relative_file_path);
};
