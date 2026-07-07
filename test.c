// example.c
#include <stdio.h>
// #include <omp.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

int main() {
    signal(SIGCHLD, SIG_IGN);
    int i = 1;
    printf("PID: %d\n", getpid());
    // omp_set_num_threads(8);
    
    // #pragma omp parallel for
    // for (i = 0; i < 100; i++) {
        while(1) {
            size_t size = 1024*1024*10;
            void *ptr = malloc(size);
            printf("итерация %d\n", i++);
            sleep(2);
        }
    //}
    return 0;
}
/* DMDCP 
СИГНАЛЫ, как обойти убивание сигнала
ULFN
Fork - функция */