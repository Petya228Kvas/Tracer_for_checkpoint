#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <sys/eventfd.h> // Или можно использовать pipe

int efd; // Дескриптор eventfd для безопасной сигнализации между потоками

// Фоновый поток, который будет создавать дамп
void* dumper_thread(void* arg) {
    uint64_t val;
    while (1) {
        // Ждем сигнала от обработчика
        read(efd, &val, sizeof(uint64_t));
        
        pid_t pid = getpid();
        char cmd[256];
        // Формируем команду для gcore. 
        // Имя файла дампа можно сделать уникальным (например, с временной меткой)
        snprintf(cmd, sizeof(cmd), "gcore -o /tmp/my_custom_core_%d %d", pid, pid);
        
        printf("[Dumper] Generating core dump for PID %d...\n", pid);
        system(cmd); // system() безопасна здесь, так как мы в обычном потоке
        printf("[Dumper] Core dump finished. Process continues.\n");
    }
    return NULL;
}

// Обработчик пользовательского сигнала (SIGUSR1)
void signal_handler(int signum) {
    uint64_t val = 1;
    // write() является async-signal-safe функцией
    write(efd, &val, sizeof(uint64_t)); 
}

int main() {
    // 1. Создаем eventfd для коммуникации
    efd = eventfd(0, 0);
    if (efd == -1) {
        perror("eventfd");
        return 1;
    }

    // 2. Запускаем фоновый поток для создания дампов
    pthread_t thread;
    pthread_create(&thread, NULL, dumper_thread, NULL);

    // 3. Регистрируем обработчик для SIGUSR1
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Перезапуск прерванных системных вызовов
    sigaction(SIGUSR1, &sa, NULL);

    printf("Process started. PID: %d\n", getpid());
    printf("Send signal to dump: kill -USR1 %d\n", getpid());

    // Имитация полезной работы
    while (1) {
        sleep(1);
    }

    return 0;
}