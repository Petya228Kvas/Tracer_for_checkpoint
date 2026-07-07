#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>

// Инструкция syscall в x86_64 занимет 2 байта
#define SYSCALL_OPCODE 0x050f
#define SYS_FORK 57 // Номер вызова

void restore_memory(pid_t pid, long orig_text, unsigned long rip);
void detach_process(pid_t pid);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return EXIT_FAILURE;
    }
    unsigned int traceable_pid = atoi(argv[1]);
    int status;

    // Подключение в процессу
    if (ptrace(PTRACE_ATTACH, traceable_pid, NULL, NULL) < 0) {
        perror("ptrace: PTRACE_ATTACH");
        return EXIT_FAILURE;
    }
    waitpid(traceable_pid, &status, 0);
    printf("Attached to process %d\n", traceable_pid);
    ///
    struct user_regs_struct old_regs, new_regs;
    long orig_text;
    // Сохраняем старые регистры
    if(ptrace(PTRACE_GETREGS, traceable_pid, NULL, &old_regs) < 0) {
        perror("ptrace: PTRACE_GETREGS");
        detach_process(traceable_pid);
        return EXIT_FAILURE;
    }
    printf("Original RIP: %llx\n", old_regs.rip);

    // Сохраняем оригинальный код
    orig_text = ptrace(PTRACE_PEEKTEXT, traceable_pid, (void *)old_regs.rip, NULL);
    printf("Original code at RIP: %lx\n", orig_text);

    // Записывает инструкцию syscall вместо оригинального кода
    long patch_text = (orig_text & ~0xFFFF) | SYSCALL_OPCODE;
    if(ptrace(PTRACE_POKETEXT, traceable_pid, (void *)old_regs.rip, (void *)patch_text) < 0) {
        perror("ptrace: PTRACE_POKETEXT");
        restore_memory(traceable_pid, orig_text, old_regs.rip);
        detach_process(traceable_pid);
        return EXIT_FAILURE;
    }

    // Настройка регистров для вызова fork()
    memcpy(&new_regs, &old_regs, sizeof(struct user_regs_struct));
    new_regs.rax = SYS_FORK;


    if(ptrace(PTRACE_SETREGS, traceable_pid, NULL, &new_regs) < 0) {
        perror("ptrace: PTRACE_SETREGS");
        restore_memory(traceable_pid, orig_text, old_regs.rip);
        detach_process(traceable_pid);
        return EXIT_FAILURE;
    }

    // Один шаг инструкции syscall
    if(ptrace(PTRACE_SINGLESTEP, traceable_pid, NULL, NULL) < 0) {
        perror("ptrace: PTRACE_SINGLESTEP");
        restore_memory(traceable_pid, orig_text, old_regs.rip);
        detach_process(traceable_pid);
        return EXIT_FAILURE;
    }
    waitpid(traceable_pid, &status, 0);

    // Получение регистров после вызова
    struct user_regs_struct regs_after;
    if(ptrace(PTRACE_GETREGS, traceable_pid, NULL, &regs_after) < 0) {
        perror("ptrace: PTRACE_GETREGS after syscall");
        restore_memory(traceable_pid, orig_text, old_regs.rip);
        detach_process(traceable_pid);
        return EXIT_FAILURE;
    }
    pid_t child_pid = (pid_t)regs_after.rax;
    if ((long)child_pid < 0) {
        fprintf(stderr, "[-] fork() syscall failed\n");
    }else {
        printf("[+] fork() syscall succeeded, child PID: %d\n", child_pid);
    }

    char command[60];
    sprintf(command, "sudo criu dump --shell-job -t %d -D dump/", child_pid);
    system(command); // Вызов crui для сохранения контрольной точки

    waitpid(child_pid, &status, 0);
    signal(SIGCHLD, SIG_IGN);
    // Восстановление оригинального кода в памяти и регистров 
    ptrace(PTRACE_POKETEXT, traceable_pid, (void *)old_regs.rip, (void *)orig_text);
    ptrace(PTRACE_SETREGS, traceable_pid, NULL, &old_regs);

    ptrace(PTRACE_DETACH, traceable_pid, NULL, NULL);
    printf("Detached from process %d\n", traceable_pid);
    

    return 0;
}

void restore_memory(pid_t pid, long orig_text, unsigned long rip) {
    ptrace(PTRACE_POKETEXT, pid, (void *)rip, (void *)orig_text);
    ptrace(PTRACE_SETREGS, pid, NULL, &orig_text);
}
void detach_process(pid_t pid) {
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    printf("Detached from process %d\n", pid);
}
