#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "omp.h"

#define one
//#define two
#define n 100000000

int main(int argc, char **argv){
#ifdef one
    size_t size = 1024*1024*100;

    omp_set_num_threads(8);
    #pragma omp parallel
    while(1){
        void *ptr = malloc(size);
        for(int i =0; i<1024*1024*100; i++)
            ((char*)ptr)[i]=1;
        printf("Выделено 100МБ\n");
        usleep(1000000000);
    }
#endif

#ifdef two
    double *a = malloc(n*sizeof(double));
    double *b = malloc(n*sizeof(double));
    double *c = malloc(n*sizeof(double));

    for(int i  = 0; i<n; i++){
        a[i] = 1.0;
        b[i]=2.0;
    }
    int pam = 67;
    double st = omp_get_wtime();
     sleep(100);
     #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
    double end = omp_get_wtime();
    printf("Time: %lf\n", end-st);

    free(a);
    free(b);
    free(c);
#endif

    
    return 0;
}

