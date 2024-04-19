#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> 
#include "std_lib_facilities.h"
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <pthread.h>

pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;

map<string, vector<string>> msgs; // handled in new_client_thread_func
map<string, int> sockets; // handled in listening_thread_func
vector<pthread_t> threads; // handled in listening_thread_func

string main_socket = "main_socket"; // ASK
bool has_msg = false;
bool received_msg = false;

void* new_client_thread_func(void* arg) {
    int connfd = *(int*)arg;  // new clients socket fd
    string client_name(256, 0);
    auto r = read(connfd, &client_name[0], client_name.size()); 
    client_name = client_name.substr(0, r - 1);
   
    // Mapping a new client name with his socket fd to sockets map.
    pthread_mutex_lock(&socket_mutex); 
    sockets[client_name] = connfd;
    pthread_mutex_unlock(&socket_mutex); 

    for (;;) { // After name is assigned to a socket the thread will loop here until exit will be called.
        
        pthread_mutex_lock(&msg_mutex);
        if (received_msg == true){ 
            string recv_message(256, 0);
            recv(sockets[main_socket], &recv_message[0], recv_message.size(), 0);
            send(connfd, &recv_message[0], recv_message.size(), MSG_NOSIGNAL);
            received_msg = false;
        }
        pthread_mutex_unlock(&msg_mutex);

        string new_message(256, 0);
        recv(connfd, &new_message[0], new_message.size(), 0);

        size_t pos = new_message.find(' ');
        if (pos != new_message.length()) { 
            string recipient = new_message.substr(0, pos);
            string message = new_message.substr(pos + 1);

            cout << "zz" << recipient << "zz" << endl;

            // Add message to msgs map for recipient.
            pthread_mutex_lock(&msg_mutex);
            msgs[recipient].push_back(message);
            has_msg = true;
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&msg_mutex);
        }
    }
    return NULL;
}

void* listening_thread_func(void* arg) { 
    int sockfd = *(int*)arg;
    listen(sockfd, 5); // change to infinite ASK SOMEONE! try insert inside the loop
    for(;;) {
        int connfd = accept(sockfd, NULL, NULL); // wait for "nc localhost 4444" and create new socket
        cout << "connected  " << connfd << endl;
        pthread_t new_client_thread; 
        pthread_create(&new_client_thread, NULL, new_client_thread_func, &connfd);
        threads.push_back(new_client_thread);
    }
    return NULL;
}

int main(int argc, char **argv) {

    // Open a TCP/IP socket.
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockets[main_socket] = sockfd;
    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));

    // Create Listening trhead.
    pthread_t listening_thread;
    bind(sockfd, (sockaddr*)&addr, sizeof(addr)); 
    pthread_create(&listening_thread, NULL, listening_thread_func, &sockfd);
    
    for(;;) {
        vector<string> msg_vector;
        string client_name;
        string new_msg;
        pthread_mutex_lock(&msg_mutex);
        
        while (has_msg == false) {
            pthread_cond_wait(&cond, &msg_mutex); // When thread unlock goes down 
        }

        for(auto client_msg : msgs) {
            if (!client_msg.second.empty()){ // Check if the vector has a message
                msg_vector = client_msg.second;
                client_name = client_msg.first;
                
                for (const auto& msg : msg_vector) {
                    for (char c : msg) {
                        if (c != '\n' && c != '\r') {
                            new_msg += c;
                        }
                    }
                }
                cout << "zz" << new_msg << "zz" << endl;
            }
        }
        
        msgs[client_name].clear();
        received_msg = true;
        send(sockets[client_name], &new_msg[0], new_msg.size(), MSG_NOSIGNAL);
        has_msg = false;
        pthread_mutex_unlock(&msg_mutex);
    }
    close(sockfd);
}

// README:
// Run main terminal with: 
// 1. g++ assignment3.cpp
// 2. ./a.out <port>
// Run clients:
// nc localhost <port>
// first line: client name
// next lines: 'recipient' name and the message to deliver.

