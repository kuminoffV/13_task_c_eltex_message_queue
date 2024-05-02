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
    buf->priority = 1; // Устанавливаем приоритет
    sprintf(buf->text, "Hello client from server"); // Задаем текст сообщения

    // Создаем или получаем ID очереди "server_to_client" с разрешениями 0600
    msqid_server_to_client = msgget(ftok("server", 1), IPC_CREAT | 0600);

    // Создаем или получаем ID очереди "client_to_server" с разрешениями 0600
    msqid_client_to_server = msgget(ftok("client", 2), IPC_CREAT | 0600);

    // Отправляем сообщение в очередь "server_to_client"
    msgsnd(msqid_server_to_client, buf, sizeof(struct msgbuf), 0);
    
    printf("server: "); // Префикс перед получением сообщения
    // Получаем сообщение из очереди "client_to_server"
    msgrcv(msqid_client_to_server, buf, sizeof(struct msgbuf), 0, 0);
    printf("%ld\n%s\n", buf->priority, buf->text); // Выводим приоритет и текст полученного сообщения
    
    // Удаляем очереди сообщений после завершения работы
    msgctl(msqid_server_to_client, IPC_RMID, NULL);
    msgctl(msqid_client_to_server, IPC_RMID, NULL);

    exit(EXIT_SUCCESS); // Завершаем программу успешно
}

