#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> 
#include "std_lib_facilities.h"

vector<string> msgs;
pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

vector<int> sockets;
pthread_mutex_t sock_mutex = PTHREAD_MUTEX_INITIALIZER;

vector<pthread_t> threads;

void* client_thread_func(void *arg) {
    int connfd = *(int*)arg;
    for (;;) {
        string s(256, 0); 
        auto n = read(connfd, &s[0], s.size());
        if (n == 0)
            continue;
        s = s.substr(0, n);
        cout << "received [" << s << "] " << s.size() << "\n";
        pthread_mutex_lock(&msg_mutex);
        msgs.insert(msgs.begin(), s);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&msg_mutex);
    }
    return NULL;
}

void* accepting_thread_func(void *arg) {
    int sockfd = *(int*)arg;
    for (;;) {
        int connfd = accept(sockfd, NULL, NULL);
        cout << "connected  " << connfd << endl;
        pthread_mutex_lock(&sock_mutex);
        sockets.push_back(connfd);
        pthread_mutex_unlock(&sock_mutex);
        pthread_t thread;
        pthread_create(&thread, NULL, client_thread_func, &connfd);
        threads.push_back(thread);
    }
    return NULL;
}

void server(int sockfd, sockaddr_in *addr) {    
    pthread_t accepting_thread;
    int res = bind(sockfd, (sockaddr*)addr, sizeof(*addr));   
    listen(sockfd, 5);
    pthread_create(&accepting_thread, NULL, accepting_thread_func, &sockfd);
    for (;;) {
        pthread_mutex_lock(&msg_mutex);
        while (msgs.size() == 0) //if no new msg - wait
            pthread_cond_wait(&cond, &msg_mutex);
        string s = msgs.back(); //get last msg
        msgs.pop_back();//remove last msg
        pthread_mutex_unlock(&msg_mutex);
        cout << "[" << s << "] " << s.size() << "\n";
        pthread_mutex_lock(&sock_mutex);        
        for (auto sock : sockets) {     
            cout << "transmitting to " << sock << endl;
            write(sock, &s[0], s.size());
        }
        pthread_mutex_unlock(&sock_mutex);
        if (s == "exit")
            break;
    }
    cout << "cancelling threads" << endl;
    pthread_cancel(accepting_thread);
    for (auto th : threads)
        pthread_cancel(th);
    for (auto sock : sockets)
        close(sock);
}

void* client_send_func(void* arg) {
    int sockfd = *(int*)arg;
    for (;;) {
        string s;
        getline(cin, s);
        write(sockfd, &s[0], s.size());
    }
    return NULL;
}

void client(int sockfd, sockaddr_in *addr) {
    pthread_t client_send_thread;
    connect(sockfd, (sockaddr*)addr, sizeof(*addr));
    pthread_create(&client_send_thread, NULL, client_send_func, &sockfd);
    
    for (;;) {
        string s(256, 0);
        auto n = read(sockfd, &s[0], s.size());
        s = s.substr(0, n);
        cout << "]] " << s << "\n";        
        if (s == "exit")
            break;
    }

    cout << "cancelling send thread" << endl;
    pthread_cancel(client_send_thread);
    close(sockfd);
}

int main(int argc, char **argv) {  
    string mode(argv[1]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;    
    addr.sin_port = htons(atoi(argv[3]));

    if (mode == "server") {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server(sockfd, &addr);
    } else {
        addr.sin_addr.s_addr = inet_addr(argv[2]);
        client(sockfd, &addr);
    }

    close(sockfd);
}

// run as so: g++ main.cpp; ./a.out server 127.0.0.1 4444; /a.out client 127.0.0.1 4444