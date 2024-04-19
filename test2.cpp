#include <pthread.h>
#include <unistd.h>
#include "std_lib_facilities.h"

void* f(void* p) {
    for (;;) {
        cout << "#" << endl;
        sleep(2);
    }
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, f, NULL);

    for (;;) {
        cout << "." << endl;
        sleep(1);
    }

    return 0;
}