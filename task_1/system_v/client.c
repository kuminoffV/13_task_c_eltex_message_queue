#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define BUF_SIZE 64 // Размер буфера для текстовых сообщений

// Структура сообщения с приоритетом и текстом
struct msgbuf {
    long priority; // Приоритет сообщения
    char text[BUF_SIZE]; // Текст сообщения
};

int main()
{
    int msqid_server_to_client = 0, // ID очереди сообщений от сервера к клиенту
        msqid_client_to_server = 0; // ID очереди сообщений от клиента к серверу
    struct msgbuf *buf = (struct msgbuf *)malloc(sizeof(struct msgbuf)); // Выделяем память для структуры сообщения

    // Получаем ID очереди для сообщений от сервера к клиенту
    msqid_server_to_client = msgget(ftok("server", 1), 0);

    // Получаем ID очереди для сообщений от клиента к серверу
    msqid_client_to_server = msgget(ftok("client", 2), 0);

    printf("client: "); // Печать префикса перед получением сообщения
    // Получаем сообщение из очереди "server_to_client"
    msgrcv(msqid_server_to_client, buf, sizeof(struct msgbuf), 0, 0);
    printf("%ld\n%s\n", buf->priority, buf->text); // Выводим приоритет и текст полученного сообщения

    // Заполняем текстовое поле буфера новым сообщением и отправляем его в очередь "client_to_server"
    sprintf(buf->text, "Hello server from client");
    msgsnd(msqid_client_to_server, buf, sizeof(struct msgbuf), 0);

    // Удаляем очереди сообщений после завершения работы
    msgctl(msqid_server_to_client, IPC_RMID, NULL);
    msgctl(msqid_client_to_server, IPC_RMID, NULL);

    exit(EXIT_SUCCESS); // Завершаем программу успешно
}

