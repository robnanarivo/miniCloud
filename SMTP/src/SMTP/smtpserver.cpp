#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <vector>
#include <cerrno>
#include <iostream>

#include "smtpserver.hpp"
#include "service/storageservice.hpp"

int main(int argc, char *argv[]) {
    // init globals
    for (int i = 0; i < 100; i++) {
        sockets[i] = -1;
        thd[i] = -1;
        available[i] = true;
    }

    // init lock
    if (pthread_mutex_init(&debug_lock, nullptr) != 0) {
        std::cerr << "Mutex init for debug lock has failed" << std::endl;
        return EXIT_FAILURE;
    }

    // set signal hanlder
    signal(SIGINT, shutdown_server);

    int c;
    // default port number is 2500
    int port_no = 2500;
    std::string number;

    while ((c = getopt(argc, argv, "p:v")) != -1) {
        switch (c) {
            case 'p':
                number = std::string(optarg);
                port_no = std::stoi(number);
                break;
            case 'v':
                debug_mode = true;
                break;
            default:
                std::cerr << "Usage: ./smtpserver [-p port_number] [-v]"
                          << std::endl;
                return EXIT_FAILURE;
        }
    }

    // initialize globals for email synchronization - use storage service
    // create storeclient and storage service
    // TODO add process to read master NetworkInfo
    NetworkInfo master("127.0.0.1", 8001);
    mail_info.master_ip = master.ip;
    mail_info.master_port = master.port;
    StoreClient storeClient(master);
    StorageService storeServ(storeClient);

    int count = 0;
    // .email is a row which is used to store all mbox files for users
    std::vector<FolderElement> files = storeServ.list_folder(".email", "/");
    for (const auto& entry : files) {
        mail_info.box2lock[entry.hash] = count;
        mail_info.user2box[entry.filename] = entry.hash;
        count++;
    }

    // initialize mail locks
    mail_info.mail_locks = std::make_unique<pthread_mutex_t[]>(count);
    for (int i = 0; i < count; i++) {
        if (pthread_mutex_init(&mail_info.mail_locks[i], nullptr) != 0) {
            std::cerr << "Mutex init for mailbox lock has failed" << std::endl;
            return EXIT_FAILURE;
        }
    }

    // socket creation
    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port_no);

    // allow bind to reuse port -> no need to wait for port to be released
    int opt = 1;
    int ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                         &opt, sizeof(opt));
    if (bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        debug(-1, "Bind to port " + std::to_string(port_no) +
                      " failed: " + strerror(errno));
        exit(EXIT_FAILURE);
    }

    // listen to socket for connections
    if (listen(listen_fd, 100) != 0) {
        debug(-1, "Listen to port " + std::to_string(port_no) + " failed");
        exit(EXIT_FAILURE);
    }

    debug(-1, "Server started at port " + std::to_string(port_no));

    // set listen socket to non-blocking -> allow SIGINT to terminate all threads
    set_socket_nonblock(listen_fd);

    // loop to accept incoming connections
    while (true) {
        // handle shut down signal
        if (server_shutdown) {
            debug(-1, "Server shut down");
            break;
        }

        struct sockaddr_in clientaddr;
        socklen_t clientaddrlen = sizeof(clientaddr);
        int comm_fd =
            accept(listen_fd, (struct sockaddr *)&clientaddr, &clientaddrlen);
        if (comm_fd == -1) {
            continue;
        }

        int slot_id = find_free_slot();
        if (slot_id == -1) {
            continue;
        }

        // set communication socket to non-blocking
        set_socket_nonblock(comm_fd);
        debug(comm_fd, "New connection");

        sockets[slot_id] = comm_fd;
        pthread_t thread_id;
        // freed in thread function
        int *slot_id_ptr = new int(slot_id);
        if (pthread_create(&thread_id, nullptr, handle_new_connection,
                           (void *)slot_id_ptr) != 0) {
            debug(comm_fd, "Thread creation fails");
        }
        thd[slot_id] = thread_id;
    }

    // clean up
    for (int i = 0; i < 100; i++) {
        if (thd[i] != -1 && pthread_join(thd[i], nullptr) != 0) {
            std::cerr << "Thread failed to join" << std::endl;
        }
    }
    pthread_mutex_destroy(&debug_lock);
    for (int i = 0; i < count; i++) {
        pthread_mutex_destroy(&mail_info.mail_locks[i]);
    }

    return 0;
}

int find_free_slot() {
    int slot_id = -1;
    while (slot_id == -1) {
        if (server_shutdown) {
            break;
        }
        for (int i = 0; i < 100; i++) {
            if (available[i]) {
                slot_id = i;
                available[i] = false;
                sockets[i] = -1;
                if (thd[i] != -1) {
                    // free up old resource
                    pthread_join(thd[i], nullptr);
                }
                break;
            }
        }
    }
    return slot_id;
}

void *handle_new_connection(void *slot_id_void_ptr) {
    int *slot_id_ptr = (int *)slot_id_void_ptr;
    int slot_id = *slot_id_ptr;
    int comm_fd = sockets[slot_id];
    smtp(comm_fd, slot_id);
    delete slot_id_ptr;
    return nullptr;
}

void smtp(int comm_fd, int slot_id) {
    char buffer[BUFFER_SIZE];
    int buf_end = 0;
    std::string command;
    bool seen_CR = false;
    bool terminate = false;

    Mailbox box(comm_fd, SERVER_DOMAIN, mail_info);

    while (true) {
        int rcvd = read(comm_fd, buffer, BUFFER_SIZE);
        int buf_ptr = 0;
        while (buf_ptr < rcvd) {
            // if we see <CR>, we want to check if the next char is <LF>
            if (buffer[buf_ptr] == '\r') {
                seen_CR = true;
            } else if (seen_CR) {
                // if the next char is <LF>, we parse the command
                if (buffer[buf_ptr] == '\n') {
                    if (box.parse_command(command) != -1) {
                        command = "";
                    } else {
                        terminate = true;
                        break;
                    }
                }
                // else, we treat <CR> as part of the message
                else {
                    command += '\r';
                }
                seen_CR = false;
            }
            // if the char is anything else, we treat it as part of the message
            else {
                command += buffer[buf_ptr];
            }
            buf_ptr++;
        }

        if (terminate) {
            break;
        }

        if (server_shutdown) {
            break;
        }
    }

    // close connection
    close(comm_fd);
    available[slot_id] = true;
    debug(comm_fd, "Connection closed");
}

void shutdown_server(int sig) {
    server_shutdown = true;
}

// set socket to non-blocking
void set_socket_nonblock(int socket_fd) {
    int opts = fcntl(socket_fd, F_GETFL);
    if (opts < 0) {
        fprintf(stderr, "F_GETFL failed (%s)\n", strerror(errno));
        abort();
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(socket_fd, F_SETFL, opts) < 0) {
        fprintf(stderr, "F_SETFL failed (%s)\n", strerror(errno));
        abort();
    }
}

void debug(int comm_fd, std::string message) {
    if (debug_mode) {
        pthread_mutex_lock(&debug_lock);
        if (comm_fd != -1) {
            std::cerr << "[" << comm_fd << "] " << message << std::endl;
        } else {
            std::cerr << message << std::endl;
        }
        pthread_mutex_unlock(&debug_lock);
    }
}
