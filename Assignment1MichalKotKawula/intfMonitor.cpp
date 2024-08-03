#include <iostream>
#include <fstream>
#include <signal.h>
#include <string>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include <cstdlib>
#include <errno.h>
#include <cstring>

#define GET_VARIABLE_NAME(Variable) (#Variable)
#define DEBUG

using namespace std;


//variables
char socket_path[] = "/tmp/socketConnection";
bool is_running = true;
const int BUF_LEN = 100;
string arg;


//signal handling
void handle(int sig) {
    switch (sig) {
        case SIGINT:
            is_running = false;
            break;
    }
}


//configure signals
void sigHandler() {
    struct sigaction action;
    action.sa_handler = handle;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGINT, &action, NULL);
}

// Read the statistics of files
string get_statistics(const string& file_path, const string& var_name, bool stats_path = true) {
    string value, path;
    if (stats_path) {
        path = file_path + "/statistics/" + var_name;
    } else {
        path = file_path + "/" + var_name;
    }


    if (!filesystem::exists(path)) {
        cerr << "Path not found: " << path << endl;
        return "";
    }

    ifstream file(path);
    if (file.is_open()) {
        getline(file, value);
        file.close();
    } else {
        cerr << "Unable to open file: " << path << endl;
    }
    return value;
}

// Set up link if interface is down
void setLinkUp(const string& name) {
    struct ifreq ifr;
    strcpy(ifr.ifr_name, name.c_str());
    int linkfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (linkfd >= 0) {
        if (ioctl(linkfd, SIOCGIFFLAGS, &ifr) == 0) {
            // Setting link up by its flags
            ifr.ifr_flags |= IFF_UP;
            if (ioctl(linkfd, SIOCSIFFLAGS, &ifr) == 0) {
                #ifdef DEBUG
                cout << "Set Link Up successfully." << endl;
                #endif
            } else {
                cout << "Error set link up: " << strerror(errno) << endl;
            }
        }
        close(linkfd);
    } else {
        cout << "Error: " << strerror(errno) << endl;
    }
}

int main(int argc, char *argv[]) {
    


    //Socket set up
    struct sockaddr_un addr;
    char buf[BUF_LEN];
    int len, ret;
    int fd, rc;
    string file_path;

    sigHandler();

    // if not two args
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <interface-name>" << endl;
        return 1;
    }

    arg = argv[1];
    file_path = "/sys/class/net/" + arg;

    if (!filesystem::is_directory(file_path)) {
        cout << file_path << " path not found" << endl;
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));

    // Create the socket
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        cout << "client(" << getpid() << "): Error create socket - " << strerror(errno) << endl;
        exit(-1);
    }

    addr.sun_family = AF_UNIX;
    // Set the socket path to the local socket file
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    while (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        if (errno == EINTR) {
            continue;
        }
        cout << "client(" << getpid() << "): Error connect socket - " << strerror(errno) << endl;
        close(fd);
        exit(-1);
    }

    cout << "client(" << getpid() << "): Connected to network monitor" << endl;

    write(fd, "Ready", sizeof("Ready"));
    cout << "client(" << getpid() << "): Sent Ready to network monitor" << endl;

    int bytes = read(fd, buf, sizeof(buf));
    buf[bytes] = '\0'; 
    cout << "client(" << getpid() << "): Received " << buf << " from network monitor" << endl;

    if (bytes > 0 && strcmp(buf, "Monitor") == 0) {
        write(fd, "Monitoring", sizeof("Monitoring"));
        is_running = true;
        cout << "client(" << getpid() << "): Monitoring started" << endl;
    } else {
        cerr << "client(" << getpid() << "): Unexpected message " << buf << endl;
    }

    // Set the socket to non-blocking mode
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    string operstate;
    int carrier_up_count = 0;
    int carrier_down_count = 0;
    int rx_bytes = 0;
    int rx_dropped = 0;
    int rx_errors = 0;
    int rx_packets = 0;
    int tx_bytes = 0;
    int tx_dropped = 0;
    int tx_errors = 0;
    int tx_packets = 0;

    string last_operstate;

    // Read file to get statistics
    while (is_running) {
        
        string var_name = GET_VARIABLE_NAME(operstate);
        operstate = get_statistics(file_path, var_name, false);

        var_name = GET_VARIABLE_NAME(rx_bytes);
        rx_bytes = stoi(get_statistics(file_path, var_name));

        var_name = GET_VARIABLE_NAME(rx_dropped);
        rx_dropped = stoi(get_statistics(file_path, var_name));

        var_name = GET_VARIABLE_NAME(rx_errors);
        rx_errors = stoi(get_statistics(file_path, var_name));

        var_name = GET_VARIABLE_NAME(rx_packets);
        rx_packets = stoi(get_statistics(file_path, var_name));

        var_name = GET_VARIABLE_NAME(tx_bytes);
        tx_bytes = stoi(get_statistics(file_path, var_name));

        var_name = GET_VARIABLE_NAME(tx_dropped);
        tx_dropped = stoi(get_statistics(file_path, var_name));

        var_name = GET_VARIABLE_NAME(tx_errors);
        tx_errors = stoi(get_statistics(file_path, var_name));

        var_name = GET_VARIABLE_NAME(tx_packets);
        tx_packets = stoi(get_statistics(file_path, var_name));

      //keep track of operstate up and down
        if (operstate != last_operstate) {
            if (operstate == "up") {
                carrier_up_count++;
            } else if (operstate == "down") {
                carrier_down_count++;
            }
            last_operstate = operstate;
        }

        //statistics print out
        cout << "Interface: " + arg + " state: " + operstate + " up_count: " + to_string(carrier_up_count) + " down_count: " + to_string(carrier_down_count) << endl;
        cout << "rx_bytes: " + to_string(rx_bytes) + " rx_dropped: " + to_string(rx_dropped) + " rx_errors: " + to_string(rx_errors) + " rx_packets: " + to_string(rx_packets) << endl;
        cout << "tx_bytes: " + to_string(tx_bytes) + " tx_dropped: " + to_string(tx_dropped) + " tx_errors: " + to_string(tx_errors) + " tx_packets: " + to_string(tx_packets) << endl << endl;

        if (operstate == "down") {
            cout << "Interface: Link is down" << endl;
            write(fd, "Link Down", sizeof("Link Down"));
            cout << "client(" << getpid() << "): Sent Link Down to network monitor" << endl;

            // wait for network monitor to send reply
            bytes = read(fd, buf, sizeof(buf));
            if (bytes > 0) {
                buf[bytes] = '\0'; 
                cout << "client(" << getpid() << "): Received " << buf << " from network monitor" << endl;

                if (strcmp(buf, "Set Link Up") == 0) {
                    setLinkUp(arg);
                    carrier_up_count++; // Increment up_count when link is set up
                }
            }
        }

        // Check for "Shut Down" message from networkMonitor
        bytes = recv(fd, buf, sizeof(buf), MSG_DONTWAIT | MSG_PEEK);
        if (bytes > 0) {
            buf[bytes] = '\0'; 
            if (strcmp(buf, "Shut Down") == 0) {
                bytes = read(fd, buf, sizeof(buf)); 
                cout << "client(" << getpid() << "): Received Shut Down from network monitor" << endl;
                is_running = false;
            }
        }


        sleep(1);
    }

    if (!is_running) {
        write(fd, "Done", sizeof("Done"));
        cout << "client(" << getpid() << "): Sent Done to network monitor" << endl;
    }
    close(fd);

    cout << "client(" << getpid() << "): Disconnected from network monitor" << endl;

    return 0;
}

