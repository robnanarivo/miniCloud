#include "staticservice.hpp"

StaticService::StaticService(const std::string& path_prefix)
    : path_prefix_(path_prefix) {}

StaticFile StaticService::read_file(const std::string& relative_file_path) {
    return StaticFile(path_prefix_ + relative_file_path);
}
