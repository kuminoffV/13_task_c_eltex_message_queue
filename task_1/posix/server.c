#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

int main()
{
    int priority = 0; // Переменная для приоритета сообщений

    // Удаляем существующие очереди, чтобы начать с нуля
    mq_unlink("/server_to_client");
    mq_unlink("/client_to_server");

    // Открываем или создаем очередь сообщений (от сервера к клиенту) для записи
    mqd_t mqdes_server_to_client = mq_open("/server_to_client", O_WRONLY | O_CREAT, 0600, NULL);

    // Открываем или создаем очередь сообщений (от клиента к серверу) для чтения
    mqd_t mqdes_client_to_server = mq_open("/client_to_server", O_RDONLY | O_CREAT, 0600, NULL);
    
    // Создаем структуру, чтобы получить атрибуты очереди
    struct mq_attr *attr = (struct mq_attr *)malloc(sizeof(struct mq_attr));
    mq_getattr(mqdes_server_to_client, attr); // Получаем атрибуты очереди
    char *buf = (char *)malloc(sizeof(char) * attr->mq_msgsize); // Выделяем буфер для сообщений

    // Подготавливаем и отправляем сообщение (от сервера к клиенту)
    sprintf(buf, "Hi client from server");
    printf("Send message\n");
    mq_send(mqdes_server_to_client, buf, strlen(buf), 0);

    printf("Waiting message\n"); // Сообщение о том, что сервер ожидает сообщения
    memset(buf, 0, sizeof(char) * attr->mq_msgsize); // Очищаем буфер
    // Получаем сообщение из очереди (от клиента к серверу)
    mq_receive(mqdes_client_to_server, buf, attr->mq_msgsize + 1, &priority);
    printf("Message: %s\n", buf); // Выводим полученное сообщение

    // Закрываем очереди сообщений
    mq_close(mqdes_server_to_client);
    mq_close(mqdes_client_to_server);

    exit(EXIT_SUCCESS); // Завершаем программу
}

