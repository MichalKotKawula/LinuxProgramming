#include <errno.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

const int PORT = 4201;
const int BUFFER_SIZE = 4096;
bool is_running = true;
int socket_fd;
char buffer[BUFFER_SIZE];
struct sockaddr_in server_address, client_address;
socklen_t client_address_len = sizeof(client_address);
pthread_mutex_t mutex_lock;
pthread_t thread_receiver;
const char *log_file_path = "/tmp/LogFile";


//Handle received data in separate threads
void* receiveData(void *arg) {
    int thread_socket_fd = *(int *)arg;

    // Timeout socket if not received anything
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    setsockopt(thread_socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

    ofstream log_file(log_file_path, ofstream::out | ofstream::app);

    while (is_running) {
        memset(buffer, 0, BUFFER_SIZE);

        pthread_mutex_lock(&mutex_lock);
        
        if (!log_file.is_open()) {
            std::cerr << "Error opening log file." << std::endl;
            exit(-1);
        }
        
        //Receive data from thread
        int message_length = recvfrom(thread_socket_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_address, &client_address_len);
        if (message_length > 0) {
            // Write data from the buffer into the file
            log_file.write(buffer, strlen(buffer));
        } else {
            sleep(1);
        }
        
        pthread_mutex_unlock(&mutex_lock);
    }
    
    log_file.close();
    return nullptr;
}

void handleSignal(int signal) {
    if (signal == SIGINT) {
        is_running = false;
    }
}


//Register signal handler for handling signals
void registerSignalHandler() {
    struct sigaction signal_action;
    signal_action.sa_handler = handleSignal;
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_flags = 0;
}


//Allow user to change the log level severity
void changeLogLevel() {
    string level;
    cout << "Choose log severity level:" << endl << "-------------------" << endl;
    cout << "1. DEBUG" << endl;
    cout << "2. WARNING" << endl;
    cout << "3. ERROR" << endl;
    cout << "4. CRITICAL" << endl;

    int user_choice;
    cin >> user_choice;

    switch (user_choice) {
        case(1):
            level = "DEBUG";
            break;
        case(2):
            level = "WARNING";
            break;
        case(3):
            level = "ERROR";
            break;
        case(4):
            level = "CRITICAL";
            break;
        default:
            cout << "Option not available. Please choose again" << endl;
            break;
    }
    memset(buffer, 0, BUFFER_SIZE);
    int length = sprintf(buffer, "Set Log Level=%s", level.c_str()) + 1;
    
    //Send log level to client socket address
    int ret = sendto(socket_fd, buffer, length, 0, (struct sockaddr *)&client_address, client_address_len);
    
    if (ret < 0) {
        perror("ERROR: Send log to client");
    } else {
        cout << "Sent to client socket this log level: " << buffer << endl;
    }
}

void displayLogFile() {
    //open file for reading
    ifstream log_file(log_file_path);

    if (!log_file.is_open()) {
        cout << "ERROR: Open log file " << strerror(errno) << endl;
        exit(-1);
    }

    //Read and display content of the file
    string line;
    while(getline(log_file, line)) {
        cout << line << endl;
    }
  
    log_file.close();
}


//Main function
int main(void) {
    // register shut down handler to listen for Ctrl C
    registerSignalHandler();

    // Create a non-blocking socket for UDP communications
    socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd < 0) {
            cout << "ERROR: Cannot create the socket: " << strerror(errno) << endl;
            return -1;
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to its IP address and to an available port.
    if (bind(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        cout << "ERROR: binding server socket: " << strerror(errno) << endl;
        exit(-1);
    }
    // Print the IP address and port after socket creation
    char ip_address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(server_address.sin_addr), ip_address, INET_ADDRSTRLEN);
    printf("LogServer listening on IP: %s, Port: %d\n", ip_address, ntohs(server_address.sin_port));
    fcntl(socket_fd, F_SETFL, O_NONBLOCK);

    // create a mutex and apply mutexing to any shared resources.
    pthread_mutex_init(&mutex_lock, NULL);

    //Start a receive thread and pass the file descriptor to it.
    int thread_create_result = pthread_create(&thread_receiver, NULL, receiveData, &socket_fd);
    if (thread_create_result != 0) {
        cout << "ERROR: Cannot create thread: " << strerror(errno) << endl;
        exit(-1);
    }

    // Run until user hits shutdown
    while(is_running) {
    
    //present the user with three options via a user menu: 
        cout << "Choose the following options:" << endl << "-------------------" << endl;
        cout << "1. Set log level" << endl;
        cout << "2. Dump log file" << endl;
        cout << "0. Shut down" << endl;

        int user_choice;
        cin >> user_choice;

        switch (user_choice) {
            case(1):
                changeLogLevel();
                break;
            case(2):
                displayLogFile();
                break;
            case(0):
                is_running = false;
                break;
            default:
                cout << "Option not available. Please choose again" << endl;
        }
    }

    //Close thread, mutex, and file descriptor
    pthread_join(thread_receiver, NULL);
    pthread_mutex_destroy(&mutex_lock);
    close(socket_fd);

    return 0;
}

