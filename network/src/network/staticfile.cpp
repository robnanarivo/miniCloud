#include "staticfile.hpp"
#include <filesystem>

StaticFile::StaticFile(const std::string& file_path) {
    if (std::filesystem::exists(file_path)) {

        // set filename
        size_t slash_pos = file_path.find('/');
        if (slash_pos == std::string::npos) {
            filename = file_path;
        } else {
            filename = file_path.substr(slash_pos + 1, std::string::npos);
        }

        // set body
        std::ifstream infile(file_path);
        //get length of file
        infile.seekg(0, std::ios::end);
        size_t length = infile.tellg();
        infile.seekg(0, std::ios::beg);
        data = DataChunk(length);
        infile.read((char*)data.data.get(), length);
        infile.close();

        // set type
        std::string extension = filename.substr(filename.find_last_of('.') + 1, std::string::npos);
        if (extension_mapping.find(extension) != extension_mapping.end()) {
            type = extension_mapping.at(extension);
        } else {
            type = "text/plain";
        }
    } else {
        throw std::runtime_error("StaticFile: no such file: " + file_path);
    }
}

StaticFile::StaticFile(const std::string& filename, const std::string& type, DataChunk data)
    : filename(filename),
      type(type),
      data(data) {}
std::string StaticFile::parse_content_type(const std::string& path) {
    std::string extension = path.substr(path.find_last_of('.') + 1, std::string::npos);
    if (extension_mapping.find(extension) != extension_mapping.end()) {
        return extension_mapping.at(extension);
    } else {
        return "text/plain";
    }
}
const std::unordered_map<std::string, std::string> StaticFile::extension_mapping = {
    {"htm", "text/html"},
    {"html", "text/html"},
    {"css", "text/css"},
    {"js", "text/javascript"},
    {"json", "application/json"},
    {"png", "image/png"},
    {"gif", "image/gif"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"mp3", "audio/mpeg"},
    {"wav", "audio/wav"},
    {"mp4", "video/mp4"},
    {"avi", "video/x-msvideo"},
    {"zip", "application/zip"},
    {"bmp", "image/bmp"},
    {"csv", "text/csv"},
    {"doc", "application/msword"},
    {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {"gz", "application/gzip"},
    {"ico", "image/vnd.microsoft.icon"},
    {"pdf", "application/pdf"},
    {"txt", "text/plain"},
};
