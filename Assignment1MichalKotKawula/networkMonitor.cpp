#include <iostream>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG

using namespace std;


//variables
char socket_path[] = "/tmp/socketConnection";
bool is_running;
const int BUF_LEN = 100;
const int MAX_CLIENTS = 2;
vector<int> clients;
vector<int> childPids;


//Handle signals
static void sigHandler(int sig) {
    switch (sig) {
        case SIGINT:
            is_running = false;
            break;
    }
}


//configure signals
void setupSigHandler() {
    struct sigaction action;
    action.sa_handler = sigHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGINT, &action, NULL);
}


//shut down existing clients
void shutdownClients() {
    char shutdown_msg[] = "Shut Down";
    for (int client : clients) {
        write(client, shutdown_msg, sizeof(shutdown_msg));
        close(client);
    }
    unlink(socket_path);
}

//send signal to kill children processes
void killChildren() {
    for (int pid : childPids) {
        kill(pid, SIGINT);
    }
}

int main(int argc, char *argv[]) {
    
    struct sockaddr_un addr;
    int master_fd;
    fd_set readfds, activefds;
    int ret;
    char buf[BUF_LEN];

    cout << "DEBUG - pid: " << getpid() << endl;

    setupSigHandler();

    // Fork child processes
    for (int i = 1; i < argc; i++) {
        int child = fork();

        if (child == -1) {
            cout << "Server - Error fork child: " << strerror(errno) << endl;
            exit(-1);
        } else if (child == 0) {
            execl("./intfMonitor", argv[i], (char *)NULL);
        } else {
            childPids.push_back(child);
        }
    }

    // Create the socket
    cout << "DEBUG - server: create socket" << endl;
    memset(&addr, 0, sizeof(addr));
    if ((master_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        cout << "server - Error create master socket: " << strerror(errno) << endl;
        exit(-1);
    }

    addr.sun_family = AF_UNIX;
    // Set the socket path to the local socket file
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    unlink(socket_path);

    // Bind the socket 
    if (bind(master_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
        cout << "server: " << strerror(errno) << endl;
        close(master_fd);
        exit(-1);
    }
      
    //Wait for client
    cout << "Waiting for the client..." << endl;
    if (listen(master_fd, argc - 1) == -1) {
        cout << "server: " << strerror(errno) << endl;
        unlink(socket_path);
        close(master_fd);
        exit(-1);
    }

    FD_ZERO(&readfds);
    FD_ZERO(&activefds);

    //set file descriptor
    FD_SET(master_fd, &readfds);
    int max_fd = master_fd;

    cout << "client(s) connected to the server" << endl;
    is_running = true;


// while client is connected
    while (is_running) {
        activefds = readfds;

        if (select(max_fd + 1, &activefds, NULL, NULL, NULL) == -1) {
            if (errno == EINTR) {
                continue;  
            }
            cout << "server - Error: select " << strerror(errno) << endl;
            shutdownClients();
            close(master_fd);
            exit(-1);
        }

        // Incoming connection
        if (FD_ISSET(master_fd, &activefds)) {
        
        
            // Accept client's connection
            int clientSoc = accept(master_fd, NULL, NULL);
            if (clientSoc == -1) {
                cout << "server - Error cannot accept client socket: " << strerror(errno) << endl;
                shutdownClients();
                close(master_fd);
                exit(-1);
            }
            FD_SET(clientSoc, &readfds);
            if (clientSoc > max_fd) {
                max_fd = clientSoc;
            }
            clients.push_back(clientSoc);
            cout << "server: client connected" << endl;
        }

        // Handling client communication
        for (size_t i = 0; i < clients.size(); ++i) {
            int cl_soc = clients[i];

            if (FD_ISSET(cl_soc, &activefds)) {
                int bytes = read(cl_soc, buf, sizeof(buf));
                if (bytes <= 0) {
                    if (bytes == 0) {
                        cout << "server: client disconnected" << endl;
                    } else {
                        cout << "server: Error reading from client socket: " << strerror(errno) << endl;
                    }
                    close(cl_soc);
                    FD_CLR(cl_soc, &readfds);
                    clients.erase(clients.begin() + i);
                    continue;
                }
                buf[bytes] = '\0';  
                cout << "server: Received " << buf << " from client" << endl;

                if (strcmp(buf, "Ready") == 0) {
                    write(cl_soc, "Monitor", sizeof("Monitor"));
                    cout << "server: Monitoring command sent to client" << endl;
                }
                if (strcmp(buf, "Link Down") == 0) {
                    write(cl_soc, "Set Link Up", sizeof("Set Link Up"));
                    cout << "server: Set Link Up command sent to client" << endl;
                }
                if (strcmp(buf, "Done") == 0) {
                    close(cl_soc);
                    FD_CLR(cl_soc, &readfds);
                    clients.erase(clients.begin() + i);
                    cout << "server: client disconnected" << endl;
                }
            }
        }
    }

    // while no longer running close clients and kill child processes
    shutdownClients();
    close(master_fd);

   
    killChildren();

    return 0;
}

