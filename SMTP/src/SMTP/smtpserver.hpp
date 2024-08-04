#pragma once

#include <pthread.h>

#include <string>

#include "mailbox.hpp"

#define BUFFER_SIZE 1000

// global variable indicating whether the server should shut down
volatile bool server_shutdown = false;

// globals for threads
// global array to store socket for 100 connections
volatile int sockets[100];
// global array to store pthread_t for 100 connections
volatile pthread_t thd[100];
// global array to indicate whether we can store a new connection at a given index
volatile bool available[100];

// global for synchronizing writing to email
struct MailInfo mail_info;

// print debug output to std err
void debug(int comm_fd, std::string message);

// find a free slot in globals to store new connection
int find_free_slot();

// wrapper for calling echo in new thread
void *handle_new_connection(void *slot_id_void_ptr);

// accept SMTP commands
void smtp(int comm_fd, int slot_id);

// set global variable server_shutdown
void shutdown_server(int sig);

// set socket system calls to non-blocking
void set_socket_nonblock(int socket_fd);

// debug functionality
// global variable for debug mode
volatile bool debug_mode = false;

// global lock for debugging output
pthread_mutex_t debug_lock;

// print debug messages
void debug(int comm_fd, std::string message);