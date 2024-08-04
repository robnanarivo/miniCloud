#include "storageservice.hpp"

#include <thread>

#include <iostream>

StorageService::StorageService(StoreClient &storeClient)
    : storeClient_(storeClient) {}

bool StorageService::isFolder(ByteArray data) 
{
    std::string content = data2string(data);
    std::string header = split(content, std::regex("\n+"))[0];
    if (header == "### FoLdEr ###") {
        return true;
    }
    return false;
}

bool StorageService::isFolderEmpty(ByteArray data)
{
    std::string content = data2string(data);
    std::vector<std::string> folder_files = split(content, std::regex("\n+"));
    if (folder_files.size() == 1) {
        return true;
    }
    return false;
}

std::string StorageService::getFileCol(ByteArray data, std::string target)
{
    if (!isFolder(data)) {
        throw std::invalid_argument("Given ByteArray is not a folder");
    }

    std::string folder = data2string(data);
    std::vector<std::string> files = split(folder, std::regex("\n+"));
    // skip the first line
    for (int i = 1; i < files.size(); i++) {
        std::vector<std::string> file_data = split(files[i], std::regex("\\s+"));
        // if target is found
        if (target == file_data[0]) {
            return file_data[1];
        }
    }
    
    // target is not found in folder
    throw std::invalid_argument("File does not exist in folder: " + target);
}

std::string StorageService::getPathCol(std::string row, std::string path)
{
    std::deque<std::string> path_q;
    for (auto &elem : split(path, std::regex("/+"))) {
        std::cout << elem << std::endl;
        path_q.push_back(elem);
    }

    std::string curr_col = ".";
    while (path_q.size() != 0) {
        std::string next_file = path_q.front();
        path_q.pop_front();
        TabletKey next_cell(row, curr_col);
        ByteArray data = storeClient_.get(next_cell);
        // if a given column does not exist in storage
        if (data.size() == 0) {
            throw std::invalid_argument("File not found in storage: " + next_file);
        }

        // get the corresponding column of a file
        curr_col = getFileCol(data, next_file);
    }
    return curr_col;
}

ByteArray StorageService::read_data(std::string row, std::string path) {
    std::string col = getPathCol(row, path);
    TabletKey key(row, col);
    std::cout << row << " " << col << std::endl;
    ByteArray data;
    try {
        data = storeClient_.get(key);
    } catch (const std::exception& e) {
        // if a given column does not exist in storage
        throw std::invalid_argument("File " + path + " not found in storage");
    }
    return data;
}

ByteArray StorageService::read_file(std::string row, std::string path) 
{
    ByteArray data = read_data(row, path);
    if (isFolder(data)) {
        throw std::invalid_argument("Given path links to a folder");
    }
    return data;
}

bool StorageService::write_file(std::string row, std::string parent_folder_path, std::string file_name, ByteArray data)
{
    std::string col = getPathCol(row, parent_folder_path);
    TabletKey key(row, col);
    ByteArray parent_folder_data = storeClient_.get(key);
    if (!isFolder(parent_folder_data)) {
        throw std::invalid_argument("Given path is not a folder");
    }

    // store file
    std::string file_hash = getHash(parent_folder_path + file_name);
    TabletKey file_key(row, file_hash);
    if (!storeClient_.put(file_key, data)) {
        return false;
    }

    // update folder
    std::string folder = data2string(parent_folder_data);
    folder += file_name + " " + file_hash + "\n";
    ByteArray updated_folder = string2data(folder);
    return storeClient_.put(key, updated_folder);
}

bool StorageService::cond_write_file(std::string row, std::string parent_folder_path, std::string file_name, ByteArray data, ByteArray old_data)
{
    std::string col = getPathCol(row, parent_folder_path);
    TabletKey key(row, col);
    ByteArray parent_folder_data = storeClient_.get(key);
    if (!isFolder(parent_folder_data)) {
        throw std::invalid_argument("Given path is not a folder");
    }

    // store file
    std::string file_hash = getHash(parent_folder_path + file_name);
    TabletKey file_key(row, file_hash);
    if (!storeClient_.cput(file_key, old_data, data)) {
        return false;
    }

    // update folder
    std::string folder = data2string(parent_folder_data);
    folder += file_name + " " + file_hash + "\n";
    ByteArray updated_folder = string2data(folder);
    return storeClient_.put(key, updated_folder);
}

bool StorageService::delete_file(std::string row, std::string path)
{    
    std::string parent_folder;
    std::string filename;
    
    std::string col = getPathCol(row, path);
    TabletKey key(row, col);
    ByteArray data = storeClient_.get(key);
    // should not delete non-empty folder
    if (isFolder(data)) {
        if (!isFolderEmpty(data)) {
            return false;
        }
        std::string raw_path = path.substr(0, path.length() - 1);
        parent_folder = raw_path.substr(0, raw_path.find_last_of("/"));
        filename = raw_path.substr(raw_path.find_last_of("/") + 1);
    } else {
        parent_folder = path.substr(0, path.find_last_of("/"));
        filename = path.substr(path.find_last_of("/") + 1);
    }

    if (!storeClient_.dele(key)) {
        return false;
    }

    // update parent folder
    return remove_entry_from_folder(row, parent_folder, filename);
}

bool StorageService::create_folder(std::string row, std::string parent_folder_path, std::string folder_name)
{
    // create a text file that represents a folder
    std::string folder = "### FoLdEr ###\n";
    ByteArray new_folder = string2data(folder);
    return write_file(row, parent_folder_path, folder_name, new_folder);
}

std::vector<FolderElement> StorageService::list_folder(std::string row, std::string path)
{
    ByteArray data = read_data(row, path);

    if (!isFolder(data)) {
        throw std::invalid_argument("Given path is not a folder");
    }

    std::vector<FolderElement> file_list;
    std::string folder = data2string(data);
    std::vector<std::string> files = split(folder, std::regex("\n+"));
    // skip the first line
    for (int i = 1; i < files.size(); i++) {
        std::vector<std::string> file_data = split(files[i], std::regex("\\s+"));
        FolderElement elem(file_data[0], file_data[1]);
        file_list.push_back(elem);
    }
    
    return file_list;
}

bool StorageService::initialize_row(std::string row)
{
    TabletKey key(row, ".");
    try {
        storeClient_.get(key);
    } catch (const std::runtime_error &e) {
        std::string root_folder = "### FoLdEr ###\n";
        ByteArray new_root_folder = string2data(root_folder);
        if (!storeClient_.put(key, new_root_folder)) {
            throw std::runtime_error("cannot create root folder");
        }
        return create_folder(row, "/", ".tmp");
    }
    throw std::invalid_argument("Given row already exists");
}

bool StorageService::move_file(std::string row, std::string from, std::string to, std::string new_name)
{
    ByteArray data = read_data(row, from);

    if (isFolder(data)) {
        throw std::invalid_argument("Given path is a folder but not a file");
    }

    if (!delete_file(row, from)) {
        return false;
    }

    return write_file(row, to, new_name, data);
}

bool StorageService::move_folder(std::string row, std::string from, std::string to, std::string new_folder)
{
    ByteArray data = read_data(row, from);

    if (!isFolder(data)) {
        throw std::invalid_argument("Given path is not a folder: " + to);
    }

    if (!create_folder(row, to, new_folder)) {
        throw std::invalid_argument("Cannot create folder at given path: " + to);
    }
    std::string new_folder_path = to + new_folder + "/";

    std::vector<FolderElement> files = list_folder(row, from);

    for (const FolderElement &file : files) {
        try {
            move_file(row, from + file.filename, new_folder_path, file.filename);
        } catch (std::invalid_argument e) {
            move_folder(row, from + file.filename + "/", new_folder_path + "/", file.filename);
        }
    }

    if (!delete_file(row, from)) {
        throw std::invalid_argument("Cannot delete original folder");
    }

    return true;
}

 bool StorageService::rename_folder(std::string row, std::string folder_path, std::string new_folder_name)
 {
    // first, get rid of the last /
    // then find the second to last / using find_last_of as delim_index
    // get the substring from start to delim_index as parent_folder_path
    std::string raw_path = folder_path.substr(0, folder_path.length() - 1);
    std::string parent_folder_path = raw_path.substr(0, raw_path.find_last_of("/"));

    if (!move_folder(row, folder_path, "/.tmp/", new_folder_name)) {
        throw std::runtime_error("Cannot copy folder to tmp store");
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (!move_folder(row, "/.tmp/" + new_folder_name + "/", parent_folder_path, new_folder_name)) {
        throw std::runtime_error("Cannot copy folder back to user");
    }

    return true;
 }

bool StorageService::remove_entry_from_folder(std::string row, std::string folder_path, std::string target)
{    
    ByteArray folder_data = read_data(row, folder_path);
    std::string col = getPathCol(row, folder_path);

    if (!isFolder(folder_data)) {
        throw std::invalid_argument("Given ByteArray is not a folder");
    }

    std::string folder = data2string(folder_data);
    std::string updated_folder = "### FoLdEr ###\n";
    std::vector<std::string> files = split(folder, std::regex("\n+"));
    // skip the first line
    for (int i = 1; i < files.size(); i++) {
        std::vector<std::string> file_data = split(files[i], std::regex("\\s+"));
        // if target is found
        if (target == file_data[0]) {
            continue;
        }
        std::string entry = files[i] + "\n";
        updated_folder += entry;
    }

    TabletKey key(row, col);
    return storeClient_.put(key, string2data(updated_folder));
}

