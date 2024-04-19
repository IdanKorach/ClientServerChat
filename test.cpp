#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "std_lib_facilities.h"

int main(int argc, char **argv) {
    int fd = socket(AF_INET, SOCK_STREAM, 0); //TCP/IP
    cout << argv[1] << endl;
    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4447);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    bind(fd, (sockaddr*)&addr, sizeof(addr));
    listen(fd, 5);
    for (;;) {
        int fd2 = accept(fd, NULL, NULL);
        if (fork() == 0) {
            for (int i = 0; i < 8; ++i) {
                string s = to_string(i) + "\n";
                write(fd2, &s[0], s.size());
                sleep(1);
            }
            close(fd2);
            exit(0);
        }
    }   

    close(fd);
}