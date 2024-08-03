// server.cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>
#include <queue>
#include <signal.h>

using namespace std;

const int BUFFER_SIZE = 4096;
const int MAX_CONN = 3;

bool is_running = true;
queue<string> msgQueue;
pthread_mutex_t msgQueueMutex = PTHREAD_MUTEX_INITIALIZER;


//Signal handler function that sets is_running to false when a SIGINT (Ctrl+C) signal is received
static void signalHandler(int signal) {
    if (signal == SIGINT) {
        is_running = false;
    }
}


//Extract the connection file descriptor from the argument passed to the receive thread.
void *receiveThread(void *arg) {
    int client_fd = *(int *)arg;
    char buffer[BUFFER_SIZE];

    // Use setsockopt() to set the read timeout to 5 seconds.
    struct timeval read_timeout;
    read_timeout.tv_sec = 5;
    read_timeout.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&read_timeout, sizeof(read_timeout));
    
    
    //infinite while loop to read data from client socket to buffer
    while (is_running) {
        int bytes_received = read(client_fd, buffer, BUFFER_SIZE - 1);

        // If there is something on the read, add it to the message queue. 
        //Be sure to mutex this message queue since it is used in main() as well.
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            pthread_mutex_lock(&msgQueueMutex);
            msgQueue.push(string(buffer));
            pthread_mutex_unlock(&msgQueueMutex);
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cout << "Usage: server <port number>" << endl;
        return -1;
    }

    cout << "Server is running..." << endl;

    // The communications stop when a ctrl-C is issued to the server. 
    //The signal handler for the server will set is_running to false 
    //causing the infinite while-loops in the receive thread and in main() to end.
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    int master_socket, client_sockets[MAX_CONN];
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    pthread_t client_threads[MAX_CONN];

    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cout << "Server: " << strerror(errno) << endl;
        exit(-1);
    }

    // Set master socket to non-blocking
    int socket_flags = fcntl(master_socket, F_GETFL, 0);
    fcntl(master_socket, F_SETFL, socket_flags | O_NONBLOCK);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // Bind the socket
    if (bind(master_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cout << "Server: " << strerror(errno) << endl;
        close(master_socket);
        exit(-1);
    }

    // Listen for incoming connections
    if (listen(master_socket, MAX_CONN) < 0) {
        cout << "Server: " << strerror(errno) << endl;
        close(master_socket);
        exit(-1);
    }

    int active_clients = 0;


//Starts a loop to accept and handle client connections. 
    while (is_running) {
        if (active_clients < MAX_CONN) {
            client_sockets[active_clients] = accept(master_socket, (struct sockaddr *)&server_addr, &addr_len);


//checks if the number of active clients is less than the maximum allowed 
            if (client_sockets[active_clients] > 0) {
                // Create a thread to handle the client
                if (pthread_create(&client_threads[active_clients], NULL, receiveThread, &client_sockets[active_clients]) != 0) {
                    cout << "Failed to create receive thread" << endl;
                    cout << strerror(errno) << endl;
                    close(client_sockets[active_clients]);
                    return -1;
                }
                active_clients++;
            }
        }

        // Print messages from the queue
        while (!msgQueue.empty()) {
            pthread_mutex_lock(&msgQueueMutex);
            cout << msgQueue.front() << endl;
            msgQueue.pop();
            pthread_mutex_unlock(&msgQueueMutex);
        }

        sleep(1);
    }

    // The server in its main() function will then send "Quit" to each client, 
    //close all connections, and exit.
    for (int i = 0; i < active_clients; ++i) {
        write(client_sockets[i], "Quit", strlen("Quit"));
        pthread_join(client_threads[i], NULL);
        close(client_sockets[i]);
    }

    cout << endl << "Server is stopping..." << endl;
    close(master_socket);

    return 0;
}
