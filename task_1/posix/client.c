#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 256

int main()
{
    int priority = 0; // Переменная для приоритета сообщений в очереди

    // Открываем очередь сообщений для чтения (от сервера к клиенту)
    mqd_t mqdes_server_to_client = mq_open("/server_to_client", O_RDONLY);

    // Открываем очередь сообщений для записи (от клиента к серверу)
    mqd_t mqdes_client_to_server = mq_open("/client_to_server", O_WRONLY);

    // Создаем структуру, чтобы получить атрибуты очереди
    struct mq_attr *attr = (struct mq_attr *)malloc(sizeof(struct mq_attr));
    mq_getattr(mqdes_server_to_client, attr); // Получаем атрибуты очереди
    char *buf = (char *)malloc(sizeof(char) * attr->mq_msgsize); // Выделяем буфер для сообщений

    printf("Waiting message\n"); // Сообщение о том, что клиент ожидает сообщения
    // Получаем сообщение из очереди (от сервера к клиенту)
    mq_receive(mqdes_server_to_client, buf, attr->mq_msgsize + 1, &priority);
    printf("Message: %s\n", buf); // Выводим полученное сообщение

    printf("Send message\n"); // Сообщение о том, что клиент отправляет сообщение
    // Подготовка и отправка сообщения (от клиента к серверу)
    sprintf(buf, "Hello server from client");
    mq_send(mqdes_client_to_server, buf, strlen(buf), 0);

    // Закрываем очереди сообщений
    mq_close(mqdes_server_to_client);
    mq_close(mqdes_client_to_server);

    exit(EXIT_SUCCESS); // Завершаем программу
}

