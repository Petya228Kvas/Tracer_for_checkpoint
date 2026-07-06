#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/types.h>

// Инструкция 'syscall' в x86_64 занимает 2 байта: 0x0f, 0x05
#define SYSCALL_OPCODE 0x050f 
#define SYS_FORK 57 // Номер sys_fork для x86_64

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <PID целевого процесса>\n", argv[0]);
        return 1;
    }

    pid_t target_pid = atoi(argv[1]);
    int status;
    struct user_regs_struct old_regs, new_regs;
    long orig_text;

    // 1. Подключаемся к целевому процессу
    if (ptrace(PTRACE_ATTACH, target_pid, NULL, NULL) < 0) {
        perror("Ошибка PTRACE_ATTACH");
        return 1;
    }
    waitpid(target_pid, &status, 0);
    printf("[+] Подключено к процессу %d\n", target_pid);

    // 2. Сохраняем оригинальные регистры
    if (ptrace(PTRACE_GETREGS, target_pid, NULL, &old_regs) < 0) {
        perror("Ошибка PTRACE_GETREGS");
        goto detach;
    }

    // 3. Сохраняем оригинальный код (8 байт по адресу RIP)
    orig_text = ptrace(PTRACE_PEEKTEXT, target_pid, (void *)old_regs.rip, NULL);
    printf("[+] Исходный код по RIP (0x%llx): 0x%lx\n", old_regs.rip, orig_text);

    // 4. Записываем инструкцию 'syscall' вместо оригинального кода
    // Подменяем только первые 2 байта, остальные сохраняем из orig_text
    long patch_text = (orig_text & ~0xFFFF) | SYSCALL_OPCODE;
    if (ptrace(PTRACE_POKETEXT, target_pid, (void *)old_regs.rip, (void *)patch_text) < 0) {
        perror("Ошибка PTRACE_POKETEXT");
        goto restore_memory;
    }

    // 5. Настраиваем регистры для системного вызова fork()
    memcpy(&new_regs, &old_regs, sizeof(struct user_regs_struct));
    new_regs.rax = SYS_FORK; // Номер системного вызова fork

    if (ptrace(PTRACE_SETREGS, target_pid, NULL, &new_regs) < 0) {
        perror("Ошибка PTRACE_SETREGS");
        goto restore_memory;
    }

    // 6. Выполняем один шаг (инструкцию syscall)
    if (ptrace(PTRACE_SINGLESTEP, target_pid, NULL, NULL) < 0) {
        perror("Ошибка PTRACE_SINGLESTEP");
        goto restore_memory;
    }
    waitpid(target_pid, &status, 0);

    // 7. Получаем регистры после вызова, чтобы узнать результат fork()
    struct user_regs_struct regs_after;
    if (ptrace(PTRACE_GETREGS, target_pid, NULL, &regs_after) < 0) {
        perror("Ошибка PTRACE_GETREGS после вызова");
        goto restore_memory;
    }

    pid_t child_pid = (pid_t)regs_after.rax;
    if ((long)child_pid < 0) {
        fprintf(stderr, "[-] Системный вызов fork() завершился ошибкой\n");
    } else {
        printf("[+] Успех! Целевой процесс создал форк с PID: %d\n", child_pid);
    }

restore_memory:
    // 8. Восстанавливаем оригинальный код в памяти
    ptrace(PTRACE_POKETEXT, target_pid, (void *)old_regs.rip, (void *)orig_text);

    // 9. Восстанавливаем оригинальные регистры
    ptrace(PTRACE_SETREGS, target_pid, NULL, &old_regs);

detach:
    // 10. Отключаемся, позволяя процессу продолжить работу
    ptrace(PTRACE_DETACH, target_pid, NULL, NULL);
    printf("[+] Отключено от процесса %d. Память и регистры восстановлены.\n", target_pid);

    return 0;
}
