#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>

#define error_func(a)                      \
    do                                     \
    {                                      \
        if (-1 == a)                       \
        {                                  \
            printf("line:%d\n", __LINE__); \
            perror("error");               \
            exit(EXIT_FAILURE);            \
        }                                  \
    } while (0)

#define PRIOR_SEND_QUERTY_OR_message 5 // приоритет (запрос на сервер)
#define PRIOR_RECEPTION 6              // приоритет (ответ от сервера)
#define MAX_USERS 10                   // максимальное количетсво пользователей
#define MAX_message 40                 // максимальное количетсво сообщейний

// мьютекс бдля добавления нового пользователя
pthread_mutex_t mutex_new_user;

// структура сообщейний пользователей и рассылки
struct send_and_request
{
    long mtype;
    char message[50];
    char users[10][20];
    int amount_users; // количетсво пользователей в сети
    int num_user;     //- номер пользователя на сервере
    int type_message; //- тип сообщения
                      //-1 - ответ: сервер переполнен
                      // 0 - запрос: вход
                      // 1 - запрос: выход
                      // 2 - ошибка соединения
                      // 3 - отправить новое сообщение
                      // 4 - ответ: новый пользователь
                      // 5 - ответ: новое сообщение
                      // 6 - отключить вервер
    int priority;     // приоритет сообщений для пользователя
                      // случай переполнения чата (-1)
};

int main(void)
{
    // ключ для соединения
    key_t key_message;

    // идентификатор очереди сообщений
    int id_message;

    // структура для передачи, в виде сообщения
    // другая хранит всю информацию о сеансе
    struct send_and_request send_and_request_users_data;
    struct send_and_request send_and_request;

    // название файла
    char *name_file = "file_key.txt";

    // проверка завершения дочернего процесса
    int status;
    ssize_t status_rcv;

    // идентификатор последнего пользователя
    int last_user = 0;

    // чпроверка занятости приоритетов
    int mas_prio[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int free_prio = 10;

    // счетчики
    int i = 0, i2 = 0;

    // идентификатор работы сервера
    int work = 1;

    // индентификатор пользователя на сервере
    int num_user_server;

    // проверка завершения дочернего процесса
    key_message = ftok(name_file, 6);
    error_func(key_message);
    id_message = msgget(key_message, IPC_CREAT | 0666);
    error_func(id_message);

    for (i = 5; i < 20; i++)
    {
        while (msgrcv(id_message, &send_and_request, sizeof(send_and_request) - sizeof(send_and_request.mtype), i, IPC_NOWAIT) > 0)
            ;
    }

    send_and_request.amount_users = 0;
    send_and_request_users_data.amount_users = 0;

    printf("Server has started and is starting to work\n");

    // начинаем принимать заявки от пользователей
    while (1 == work)
    {
        sleep(1);
        status_rcv = msgrcv(id_message, &send_and_request, sizeof(send_and_request) - sizeof(send_and_request.mtype), PRIOR_SEND_QUERTY_OR_message, 0);
        error_func(status_rcv);

        switch (send_and_request.type_message)
        {
        // запрос на вход
        case 0:
            printf("New user %s is trying to log in\n", send_and_request.users[0]);

            for (i = 0; i < MAX_USERS; i++)
            {
                if ((0 == mas_prio[i]) && (20 > free_prio))
                {
                    // счеткик пользователей
                    last_user++;
                    free_prio++;

                    i2 = i;

                    // количество пользователей в системе
                    send_and_request_users_data.amount_users = last_user;

                    // добавляем нового пользователя в общую структуру данных
                    strcpy(send_and_request_users_data.users[i], send_and_request.users[0]);

                    // приоритет нового пользователя
                    send_and_request_users_data.type_message = 4;

                    // сообщение, о новом пользователе
                    for (i = 0; i < MAX_USERS; i++)
                    {
                        if (0 != mas_prio[i])
                        {
                            send_and_request_users_data.mtype = mas_prio[i];
                            send_and_request_users_data.priority = mas_prio[i];
                            send_and_request_users_data.num_user = i;
                            status = msgsnd(id_message, &send_and_request_users_data, sizeof(send_and_request_users_data) - sizeof(send_and_request_users_data.mtype), 0);
                            error_func(status);
                        }
                    }

                    // приоритет для получения сообщений новому пользователю
                    send_and_request_users_data.priority = free_prio;

                    // занятость приоритета
                    mas_prio[i2] = send_and_request_users_data.priority;

                    // идентификатор пользователя
                    send_and_request_users_data.num_user = i2;

                    send_and_request_users_data.mtype = PRIOR_RECEPTION;
                    status = msgsnd(id_message, &send_and_request_users_data, sizeof(send_and_request_users_data) - sizeof(send_and_request_users_data.mtype), 0);
                    error_func(status);

                    // после добавления пользователя, выходим из цикла
                    break;
                }
                else if (i == 9)
                {
                    // если есть переполнение сессии, то сообщаем об этом пользователю
                    send_and_request.priority = -1;
                    send_and_request.mtype = PRIOR_RECEPTION;

                    status = msgsnd(id_message, &send_and_request, sizeof(send_and_request) - sizeof(send_and_request.mtype), 0);
                    error_func(status);

                    break;
                }
            }

            break;

        // запрос на выход
        case 1:

            for (i = 0; i < MAX_USERS; i++)
            {
                if (mas_prio[i] == send_and_request.priority)
                {
                    num_user_server = i;
                }
            }

            printf("User %s is trying to log out of the session\n", send_and_request_users_data.users[num_user_server]);

            // сообщаем процессу, что пользователь вышел
            send_and_request_users_data.mtype = mas_prio[num_user_server];
            send_and_request_users_data.type_message = -1;
            status = msgsnd(id_message, &send_and_request_users_data, sizeof(send_and_request_users_data) - sizeof(send_and_request_users_data.mtype), 0);
            error_func(status);

            // перемещение пользователей на место ушедшего
            strcpy(send_and_request_users_data.users[num_user_server], send_and_request_users_data.users[last_user - 1]);

            // зануляем последнего пользователя
            memset(send_and_request_users_data.users[last_user - 1], 0, sizeof(send_and_request_users_data.users[last_user - 1]));

            // удаляем пользователя из массива приоритетов
            mas_prio[num_user_server] = mas_prio[last_user - 1];
            mas_prio[last_user - 1] = 0;

            // обновление доступных мест
            last_user--;
            send_and_request_users_data.amount_users--;

            // отправляем список пользователей в сети
            send_and_request_users_data.type_message = 4;

            for (i = 0; i < MAX_USERS; i++)
            {
                if (0 != mas_prio[i])
                {
                    send_and_request_users_data.mtype = mas_prio[i];
                    send_and_request_users_data.priority = mas_prio[i];
                    send_and_request_users_data.num_user = i;
                    status = msgsnd(id_message, &send_and_request_users_data, sizeof(send_and_request_users_data) - sizeof(send_and_request_users_data.mtype), 0);
                    error_func(status);
                }
            }

            break;

        case 3:

            for (i = 0; i < MAX_USERS; i++)
            {
                if (mas_prio[i] == send_and_request.priority)
                {
                    num_user_server = i;
                }
            }

            for (i = 0; i < MAX_USERS; i++)
            {
                if (mas_prio[i] == send_and_request.priority)
                {
                    num_user_server = i;
                }
            }

            memset(send_and_request_users_data.message, 0, sizeof(send_and_request_users_data.message));
            strcat(send_and_request_users_data.message, send_and_request_users_data.users[num_user_server]);
            strcat(send_and_request_users_data.message, ": ");
            strcat(send_and_request_users_data.message, send_and_request.message);

            printf("User %s sends a message\n", send_and_request_users_data.users[num_user_server]);

            send_and_request_users_data.type_message = 5;

            for (i = 0; i < MAX_USERS; i++)
            {
                if (0 != mas_prio[i])
                {
                    send_and_request_users_data.mtype = mas_prio[i];
                    send_and_request_users_data.priority = mas_prio[i];
                    send_and_request_users_data.num_user = i;
                    status = msgsnd(id_message, &send_and_request_users_data, sizeof(send_and_request_users_data) - sizeof(send_and_request_users_data.mtype), 0);
                    error_func(status);
                }
            }
            break;

        // выключение сервера
        case 6:
            printf("Пользователь %s пытается выключить сервер\n", send_and_request.users[0]);
            send_and_request_users_data.amount_users = -1;

            for (i = 0; i < MAX_USERS; i++)
            {
                if (0 != mas_prio[i])
                {
                    send_and_request_users_data.mtype = mas_prio[i];
                    send_and_request_users_data.priority = mas_prio[i];
                    send_and_request_users_data.num_user = i;
                    status = msgsnd(id_message, &send_and_request_users_data, sizeof(send_and_request_users_data) - sizeof(send_and_request_users_data.mtype), 0);
                    error_func(status);
                }
            }

            work = 0;
            break;

        default:
            break;
        }
    }
    exit(EXIT_SUCCESS);
}
