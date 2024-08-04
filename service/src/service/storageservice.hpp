#pragma once

#include <deque>
#include <stdexcept>

#include "storeclient/storeclient.hpp"
#include "utils/utils.hpp"

struct FolderElement
{
    std::string filename;
    std::string hash;

    FolderElement(std::string filename, std::string hash)
        : filename(filename), hash(hash) {}
};

/*
Root folder:
Once the user finishes registration, a root folder under column "." will be created.
An email file will be created under the root folder of the row ".email" for each user upon registration.

Folder text file format example:
### FoLdEr ###
myfolder b200ae8cd3b94498af3129d81364a4f5
mydoc.pdf 9fed7e54b0f19b3909584ce8e9c3efd9
...
*/
class StorageService
{
private:
    StoreClient& storeClient_;
    // all folders must start with the header "### FoLdEr ###"
    bool isFolder(ByteArray data);
    // data must be a folder
    bool isFolderEmpty(ByteArray data);
    // currently assuming 1-to-1 relation between column and file
    std::string getFileCol(ByteArray data, std::string target);
    std::string getPathCol(std::string row, std::string path);
    // read a file or a folder
    ByteArray read_data(std::string row, std::string path);

public:
    StorageService(StoreClient &storeClient);

    /*
    For the following functions, path may either include the initial root
    folder or not. E.g. "/myfolders/game.pdf" is valid, and "myfolders/game.pdf"
    is also valid. If one or more folders or files on a given path does not
    exist, an exception will be thrown.
    */

    // username is row key; path is absolute and must be a file
    ByteArray read_file(std::string row, std::string path);

    // parent_folder_path must end with a backlash ("/")
    bool write_file(std::string row, std::string parent_folder_path, std::string file_name, ByteArray data);

    // similar to the function above, but uses cput instead of put
    bool cond_write_file(std::string row, std::string parent_folder_path, std::string file_name, ByteArray data, ByteArray old_data);

    // delete a file or a folder; a folder can only be deleted if it is empty
    // delete folder must end with / to indicate it is a folder
    bool delete_file(std::string row, std::string path);

    // create a text file which represents a folder
    bool create_folder(std::string row, std::string parent_folder_path, std::string folder_name);

    // list all files in a given folder
    std::vector<FolderElement> list_folder(std::string row, std::string path);

    // create all files a row needs
    bool initialize_row(std::string row);

    // move or rename a file
    // to is the destination folder
    bool move_file(std::string row, std::string from, std::string to, std::string new_name);

    // move a folder to a different path
    // new_folder is a name and should not end with /
    bool move_folder(std::string row, std::string from, std::string to, std::string new_folder);

    // cannot rename root folder
    bool rename_folder(std::string row, std::string folder_path, std::string new_folder_name);

    // remove file entry from folder
    bool remove_entry_from_folder(std::string row, std::string folder_path, std::string target);
};
