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
    message_buf rbuf, sbuf;
    size_t buflen;

    // Генерация того же ключа, что и у сервера
    if ((key = ftok("server.c", 'B')) == -1) {
        perror("ftok");
        exit(1);
    }

    // Подключение к очереди сообщений
    if ((msqid = msgget(key, 0666)) == -1) {
        perror("msgget");
        exit(1);
    }

    // Получение сообщения от сервера
    if (msgrcv(msqid, &rbuf, MSGSZ, 0, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }
    printf("Received message from the server: %s\n", rbuf.mtext);

    // Отправка ответа серверу
    strcpy(sbuf.mtext, "Hello!");
    sbuf.mtype = 2;
    buflen = strlen(sbuf.mtext) + 1;

    if (msgsnd(msqid, &sbuf, buflen, 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    return 0;
}

