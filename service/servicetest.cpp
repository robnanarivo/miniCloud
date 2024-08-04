#include <iostream>
#include <thread>

#include "service/adminservice.hpp"
#include "service/mailservice.hpp"
#include "service/storageservice.hpp"
#include "service/userservice.hpp"
#include "storeclient/storeclient.hpp"

void print_files(std::vector<FolderElement> files) {
  std::cout << "\nstart" << std::endl;
  for (auto &e : files) {
    std::cout << e.filename << " " << e.hash << std::endl;
  }
  std::cout << "end\n" << std::endl;
}

int main() {
  NetworkInfo master("127.0.0.1", 8001);
  StoreClient storeClient(master);

  StorageService storeServ(storeClient);
  MailService mailServ(storeServ);
  UserService userServ(storeServ);

  // NetworkInfo master("127.0.0.1", 8002);
  // StoreClient storeClient(master);
//   AdminService adminServ(storeClient);

  //   TabletKey key(".email", ".");
  //   std::string folder = "### FoLdEr ###\n";
  //   ByteArray new_folder = string2data(folder);
  //   storeClient.put(key, new_folder);
  //   std::cout << data2string(storeClient.get(key)) << std::endl;
  //   storeClient.cput(key, string2data("123"), string2data("223"));
  //   std::cout << data2string(storeClient.get(key)) << std::endl;
  //   storeClient.cput(key, new_folder, string2data("332"));
  //   std::this_thread::sleep_for(std::chrono::seconds(2));
  //   std::cout << data2string(storeClient.get(key)) << std::endl;
  //   std::this_thread::sleep_for(std::chrono::seconds(2));
  //   storeClient.dele(key);
  //   std::this_thread::sleep_for(std::chrono::seconds(2));
  //   std::cout << data2string(storeClient.get(key)) << std::endl;

    // storeServ.write_file(".email", "/", "miao", string2data("what"));

    // storeServ.create_folder(".email", "/", "bigboss");
    storeServ.create_folder("wyq", "/", "dest");
    storeServ.write_file("wyq", "/dest/", "wow123", string2data("big no good"));
    print_files(storeServ.list_folder("wyq", "/"));
    // print_files(storeServ.list_folder(".email", "/bigboss/"));
    storeServ.rename_folder("wyq", "/dest/", "okay");
    // std::cout << data2string(storeServ.read_file(".email", "/bigboss/wow123")) << std::endl;;
    // storeServ.move_folder("wyq", "/bigboss/", "/dest/", "bigboss");
    print_files(storeServ.list_folder("wyq", "/"));


    // storeServ.delete_file(".email", "/miao");
    // std::cout << data2string(storeClient.get(TabletKey(".email", "."))) << std::endl;
    // storeServ.delete_file(".email", "/secret/lol");
    // storeServ.delete_file(".email", "/secret/");
    // std::cout << data2string(storeClient.get(TabletKey(".email", "."))) << std::endl;

    // std::this_thread::sleep_for(std::chrono::seconds(2));
    // std::cout << data2string(storeServ.read_file(".email",
    // "/secret/test.txt"))
    //           << std::endl;

  // std::this_thread::sleep_for(std::chrono::seconds(1));
  //   std::cout << data2string(storeServ.read_file(".email", "/miao")) <<
  //   std::endl; std::cout << data2string(storeServ.read_file(".email",
  //   "/haha")) << std::endl;

  // userServ.sign_up("bob", "123");
  // if (mailServ.send_mail("rob@localhost", "bob@localhost", "HI", "Wanna
  // test this")) {
  //   std::cout << "success" << std::endl;
  // }

  // test nodes list
//   std::unordered_map<std::string, bool> nodes_ls =
//       adminServ.list_backend_server();
//   for (auto itr = nodes_ls.begin(); itr != nodes_ls.end(); ++itr) {
//     std::cout << itr->first << " " << std::boolalpha << itr->second
//               << std::endl;
//   }

//   // test files list
//   std::vector<TabletKey> files_ls =
//       adminServ.list_server_files("127.0.0.1:5003");
//   std::cout << "Files on T3: " << std::endl;
//   for (auto itr = files_ls.begin(); itr != files_ls.end(); ++itr) {
//     std::cout << itr->row << " " << itr->col << std::endl;
//   }

//   // test kill a node
//   bool kill_result = adminServ.kill_server("127.0.0.1:5002");
//   std::cout << "Kill T2: " << std::boolalpha << kill_result << std::endl;

//   // wait 50s check nodes list again
//   std::this_thread::sleep_for(std::chrono::seconds(50));
//   std::unordered_map<std::string, bool> nodes_ls_2 =
//       adminServ.list_backend_server();
//   for (auto itr = nodes_ls_2.begin(); itr != nodes_ls_2.end(); ++itr) {
//     std::cout << itr->first << " " << std::boolalpha << itr->second
//               << std::endl;
//   }

//   // test restart a node
//   bool rest_result = adminServ.reboot_server("127.0.0.1:5002");
//   std::cout << "Restart T2: " << std::boolalpha << rest_result << std::endl;

//   // wait 50s check nodes list again
//   std::this_thread::sleep_for(std::chrono::seconds(50));
//   std::unordered_map<std::string, bool> nodes_ls_3 =
//       adminServ.list_backend_server();
//   for (auto itr = nodes_ls_3.begin(); itr != nodes_ls_3.end(); ++itr) {
//     std::cout << itr->first << " " << std::boolalpha << itr->second
//               << std::endl;
//   }

//   // test files list again
//   std::vector<TabletKey> files_ls_2 =
//       adminServ.list_server_files("127.0.0.1:5002");
//   std::cout << "Files on T2: " << std::endl;
//   for (auto itr = files_ls_2.begin(); itr != files_ls_2.end(); ++itr) {
//     std::cout << itr->row << " " << itr->col << std::endl;
//   }
}