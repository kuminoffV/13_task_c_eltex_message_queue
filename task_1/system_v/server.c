#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define MSGSZ 128

typedef struct msgbuf {
    long mtype;
    char mtext[MSGSZ];
} message_buf;

int main() {
    int msqid;
    key_t key;
    message_buf sbuf, rbuf;
    size_t buflen;

    // Генерация уникального ключа
    if ((key = ftok("server.c", 'B')) == -1) {
        perror("ftok");
        exit(1);
    }

    // Создание очереди сообщений
    if ((msqid = msgget(key, 0666 | IPC_CREAT)) == -1) {
        perror("msgget");
        exit(1);
    }

    // Отправка сообщения клиенту
    strcpy(sbuf.mtext, "Hi!");
    sbuf.mtype = 1;
    buflen = strlen(sbuf.mtext) + 1;

    if (msgsnd(msqid, &sbuf, buflen, 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    // Удаление очереди сообщений
    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }

    // Получение ответа от клиента
    if (msgrcv(msqid, &rbuf, MSGSZ, 2, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }
    printf("Received response from the client: %s\n", rbuf.mtext);

    return 0;
}
