#include <iostream>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <signal.h>
#include <queue>
#include <cstring>
#include "client.h"

pthread_mutex_t lock_x;
std::queue<Message> message_queue;
bool is_running = true;
int msgid;

void sigint_handler(int sig) {
    std::cout << "Crtl + C Shutting Down" << std::endl;
    is_running = false;
}

void* recv_func(void* arg) {
    Message msg;
    while (is_running) {
        if (msgrcv(msgid, &msg, sizeof(MesgBuffer), 4, IPC_NOWAIT) == -1) {
            if (errno != ENOMSG) {
                std::cout << "Error receiving message: " << strerror(errno) << std::endl;
            }
            continue;
        }
        pthread_mutex_lock(&lock_x);
        std::cout << "Server received message from client " << msg.msgBuf.source << " to client " << msg.msgBuf.dest << ": " << msg.msgBuf.buf << std::endl;
        message_queue.push(msg);
        pthread_mutex_unlock(&lock_x);
    }
    pthread_exit(NULL);
}

int main() {
    key_t key;
    pthread_t recv_thread;

  
    signal(SIGINT, sigint_handler);

    // Unique key for communication
    key = ftok("serverclient", 65);
    if (key == -1) {
        std::cout << "Error generating key: " << strerror(errno) << std::endl;
        return -1;
    }

    // Message queue initiation
    msgid = msgget(key, IPC_CREAT);
    if (msgid == -1) {
        std::cout << "Error creating message queue: " << strerror(errno) << std::endl;
        return -1;
    }

    if (pthread_mutex_init(&lock_x, NULL) != 0) {
        std::cout << "Error initializing mutex: " << strerror(errno) << std::endl;
        return -1;
    }

    // Create receive thread
    if (pthread_create(&recv_thread, NULL, recv_func, NULL) != 0) {
        std::cout << "Error creating receive thread: " << strerror(errno) << std::endl;
        return -1;
    }

    while (is_running) {
        pthread_mutex_lock(&lock_x);
        if (!message_queue.empty()) {
            Message msg = message_queue.front();
            message_queue.pop();
            pthread_mutex_unlock(&lock_x);

        
            msg.mtype = msg.msgBuf.dest;
            std::cout << "Server dispatching message to client " << msg.msgBuf.dest << ": " << msg.msgBuf.buf << std::endl;
            if (msgsnd(msgid, &msg, sizeof(MesgBuffer), 0) == -1) {
                std::cout << "Error sending message: " << strerror(errno) << std::endl;
            }
        } else {
            pthread_mutex_unlock(&lock_x);
            usleep(10000);
        }
    }
//Close existing 3 clients
    for (int i = 1; i <= 3; ++i) {
        Message quit_msg;
        quit_msg.mtype = i;
        strcpy(quit_msg.msgBuf.buf, "Quit");
        if (msgsnd(msgid, &quit_msg, sizeof(MesgBuffer), 0) == -1) {
            std::cout << "Error sending quit message: " << strerror(errno) << std::endl;
        }
    }

    if (pthread_join(recv_thread, NULL) != 0) {
        std::cout << "Error joining receive thread: " << strerror(errno) << std::endl;
        return -1;
    }

    // end mutual exclusion
    pthread_mutex_destroy(&lock_x);

    // Set message queue to NULL
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        std::cout << "Error with ending queue: " << strerror(errno) << std::endl;
        return -1;
    }

    std::cout << "Server Shut Down" << std::endl;
    return 0;
}

