// example.c
#include <stdio.h>
// #include <omp.h>
#include <unistd.h>

int main() {
    int i = 0;
    printf("PID: %d\n", getpid());
    // omp_set_num_threads(8);
    
    // #pragma omp parallel for
    // for (i = 0; i < 100; i++) {
        while(1) {
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