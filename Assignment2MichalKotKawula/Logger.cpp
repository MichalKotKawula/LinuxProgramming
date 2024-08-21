#include <arpa/inet.h>
#include <iostream>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "Logger.h"

using namespace std;

// Forward declaration of receiveFunction
void* receiveFunction(void *arg);

// Port number for the server
const int SERVER_PORT = 4201;
const char SERVER_IP[] = "127.0.0.1"; 
const int BUFFER_SIZE = 4096;
bool is_running = true;
char message_buffer[BUFFER_SIZE];
int socket_fd;
struct sockaddr_in server_addr;
socklen_t address_length;
LOG_LEVEL current_log_level = DEBUG;
pthread_t thread_receive;
pthread_mutex_t mutex_lock;

int InitializeLog() {
    // create a non-blocking socket for UDP communications
    socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd < 0) {
        cout << "ERROR: Cannot create the socket: " << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }
    
    // Set the address and port of the server.
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    address_length = sizeof(server_addr);

    // Create a mutex to protect any shared resources.
    pthread_mutex_init(&mutex_lock, NULL);

    is_running = true;
    // Start the receive thread and pass the file descriptor to it.
    int createResult = pthread_create(&thread_receive, NULL, receiveFunction, &socket_fd);
    if (createResult < 0) {
        cout << "ERROR: Cannot create thread: " << strerror(errno) << endl;
        return -1;
    }

    // Print the IP address and port after connection initialization
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(server_addr.sin_addr), ip, INET_ADDRSTRLEN);
    printf("Logger connected to IP: %s, Port: %d\n", ip, ntohs(server_addr.sin_port));
    return 0;
}

// The receive thread is waiting for any commands from the server. So far there is only one command from the server: "Set Log Level=<level>".
void* receiveFunction(void *arg) {
    int fd = *(int *)arg;

    // if nothing received 1 sec timeout
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
    // run in an endless loop via an is_running flag
    while (is_running) {
        memset(message_buffer, 0, BUFFER_SIZE);
        
        // apply mutexing to any shared resources used within the recvfrom() function
        pthread_mutex_lock(&mutex_lock);
        int received_len = recvfrom(fd, message_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &address_length);
        if (received_len > 0) {
            string msg(message_buffer);
            size_t pos = msg.find("=");
            if (pos != string::npos) {
                string log = msg.substr(pos + 1);
                if (log == "DEBUG") current_log_level = DEBUG;
                else if (log == "WARNING") current_log_level = WARNING;
                else if (log == "ERROR") current_log_level = ERROR;
                else if (log == "CRITICAL") current_log_level = CRITICAL;
            }
        } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
            cout << "ERROR: Failed to receive message: " << strerror(errno) << endl;
        }
        pthread_mutex_unlock(&mutex_lock);
        sleep(1);
    }
    return nullptr;
}

// Set the filter log level and store in a variable global within Logger.cpp
void SetLogLevel(LOG_LEVEL level) {
    pthread_mutex_lock(&mutex_lock);
    current_log_level = level;
    pthread_mutex_unlock(&mutex_lock);
}

void Log(LOG_LEVEL level, const char *file_name, const char *function_name, int line_number, const char *log_message) {

    // compare the severity of the log to the filter log severity. The log will be thrown away if its severity is lower than the filter log severity.
    if (level >= current_log_level) {
    
        // create a timestamp to be added to the log message
        time_t current_time = time(0);
        char *date_time = ctime(&current_time);
        memset(message_buffer, 0, BUFFER_SIZE);
        char severity_levels[][16] = {"DEBUG", "WARNING", "ERROR", "CRITICAL"};
        int msg_len = sprintf(message_buffer, "%s %s %s:%s:%d %s\n", date_time, severity_levels[level], file_name, function_name, line_number, log_message) + 1;
        message_buffer[msg_len - 1] = '\0';

        // Send the log message immediately to the server via UDP
        if (sendto(socket_fd, message_buffer, msg_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            cout << "ERROR: Failed to send log message: " << strerror(errno) << endl;
        }
    }
}

// stop the receive thread via an is_running flag and close the file descriptor
void ExitLog() {
    is_running = false;
    pthread_join(thread_receive, NULL);
    pthread_mutex_destroy(&mutex_lock);
    close(socket_fd);
}

