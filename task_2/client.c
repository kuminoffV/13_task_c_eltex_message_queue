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
#include <curses.h>
#include <dirent.h>
#include <time.h>
#include <malloc.h>

// обраболтка ошибок
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

// структура сообщейний пользователей и рассылки
struct send_and_request
{
    long mtype;
    char message[50];
    char users[10][20];
    int amount_users; // количетсво пользователей
    int num_user;     //- номер пользователя
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

// прототип функции оформления окон
void start_logo(WINDOW *wnd_see_message, WINDOW *wnd_urers, WINDOW *wnd_send_message);

void sig_winch(int signo)
{
    struct winsize size;
    ioctl(fileno(stdout), TIOCGWINSZ, (char *)&size);
    resizeterm(size.ws_row, size.ws_col);
}

int main(void)
{
    // указатели на окна
    WINDOW *wnd_send_message;
    WINDOW *wnd_urers;
    WINDOW *wnd_see_message;

    // координаты
    int x_names = 0, y_names = 0;
    int x_message = 0, y_message = 0;

    // счетчик циклов
    int i;

    // код клавиши
    int put_code = 0;

    // ключ для соединения
    key_t key_message;

    // идентификатор очереди сообщений
    int id_message;

    // структура для передачи, в виде сообщения
    struct send_and_request send_and_request;

    // название файла
    char *name_file = "file_key.txt";
    // для создания нового процесса
    // для разделения поля имен и сообщений
    pid_t pid;

    // проверка завершения дочернего процесса
    int status;
    ssize_t status_rcv;
    key_message = ftok(name_file, 6);
    error_func(key_message);
    id_message = msgget(key_message, IPC_CREAT | 0666);
    error_func(id_message);

    // инициализирем  ncurses и перевод терминала в нужный режим
    initscr();

    // устанавливаем обработчик сигнала SIGWINCH
    signal(SIGWINCH, sig_winch);

    // отключаем отображение курсора
    curs_set(0);

    // отключаем отображение символов на экране
    noecho();

    // обновление экрана
    refresh();

    // создание двух окон (сообщений и списка пользователей)
    wnd_send_message = newwin((stdscr->_maxy / 10) - 1, (stdscr->_maxx / 4) - 3, (stdscr->_maxy / 4) + 22, 3);
    wnd_urers = newwin((stdscr->_maxy) - 10, (stdscr->_maxx / 3.5) - 1, 3, (stdscr->_maxx / 3) - 15 + 20 + 20);
    wnd_see_message = newwin((stdscr->_maxy / 3) + 20, (stdscr->_maxx / 4) - 3 + 25, 3, 3);
    wmove(wnd_see_message, 4, 2);
    wprintw(wnd_see_message, "Input your name");
    start_logo(wnd_see_message, wnd_urers, wnd_send_message);

    // обновление окон
    wrefresh(wnd_see_message);
    wrefresh(wnd_urers);
    wrefresh(wnd_send_message);
    echo();

    curs_set(1);

    // первый элемент массива имен -  имя пользователя, которое отправим серверу
    wmove(wnd_send_message, 1, 1);
    wgetnstr(wnd_send_message, send_and_request.users[0], sizeof(send_and_request.users[0]));

    // очистка полей после отправки сообщений
    werase(wnd_send_message);
    werase(wnd_urers);
    werase(wnd_see_message);

    // обновляем поля
    start_logo(wnd_see_message, wnd_urers, wnd_send_message);

    // обработка ошибок
    id_message = msgget(key_message, IPC_EXCL | 0666);
    error_func(id_message);

    // запрос серверу на вход
    send_and_request.mtype = PRIOR_SEND_QUERTY_OR_message;
    send_and_request.type_message = 0;
    status = msgsnd(id_message, &send_and_request, sizeof(send_and_request) - sizeof(send_and_request.mtype), 0);

    // обработка ошибок
    error_func(status);

    // убираем лишнюю информацию в окнах
    werase(wnd_send_message);
    start_logo(wnd_see_message, wnd_urers, wnd_send_message);

    // ожидание ответа сервера на подключение и получение имен пользователей
    status_rcv = msgrcv(id_message, &send_and_request, sizeof(send_and_request) - sizeof(send_and_request.mtype), PRIOR_RECEPTION, 0);

    // обработка ошибок
    error_func(status_rcv);

    // проверка подключеник серверу клиентом
    if (-1 == send_and_request.priority)
    {
        // в случае переполнения сервера, выходим
        printf("Сервер переполнен\nПока, друг мой\n");
        kill(pid, SIGTERM);
        delwin(wnd_send_message);
        delwin(wnd_urers);
        delwin(wnd_see_message);
        endwin();
        exit(EXIT_SUCCESS);
    }

    // выведем приходящую информацию и сессии
    for (i = 0; i < send_and_request.amount_users; i++)
    {
        wmove(wnd_urers, 4 + i, 1);
        wprintw(wnd_urers, "%s", send_and_request.users[i]);
        wrefresh(wnd_urers);
    }

    // будет два процесса
    // процесс для посылки сообщения
    pid = fork();
    error_func(pid);

    if (0 == pid)
    {
        // начальные координаты
        x_message = 1;
        y_message = 5;

        // убираем лишнюю информацию в окнах
        werase(wnd_urers);
        werase(wnd_see_message);
        start_logo(wnd_see_message, wnd_urers, wnd_send_message);
        wrefresh(wnd_see_message);

        // цикл для передачи данных от сервера
        while (1)
        {
            sleep(1);
            status_rcv = msgrcv(id_message, &send_and_request, sizeof(send_and_request) - sizeof(send_and_request.mtype), send_and_request.priority, 0);
            error_func(status_rcv);

            if (-1 == send_and_request.amount_users)
            {
                break;
            }
            else if (4 == send_and_request.type_message)
            {
                // обработка окна пользователей
                werase(wnd_urers);
                start_logo(wnd_see_message, wnd_urers, wnd_send_message);
                for (i = 0; i < send_and_request.amount_users; i++)
                {
                    wmove(wnd_urers, 4 + i, 1);
                    wprintw(wnd_urers, "%s", send_and_request.users[i]);
                    wrefresh(wnd_urers);
                }
            }
            else if (5 == send_and_request.type_message)
            {

                // обработка окна сообщений
                if (MAX_message + 4 == y_message)
                {
                    werase(wnd_see_message);
                    start_logo(wnd_see_message, wnd_urers, wnd_send_message);
                    x_message = 1;
                    y_message = 4;
                }

                wmove(wnd_see_message, y_message, x_message + 1);
                wprintw(wnd_see_message, "%s", send_and_request.message);
                wrefresh(wnd_see_message);
                y_message++;
            }

            wmove(wnd_send_message, 1, 1);
            wrefresh(wnd_send_message);
        }
    }
    else if (0 < pid)
    {

        send_and_request.mtype = PRIOR_SEND_QUERTY_OR_message;
        while (1)
        {
            sleep(1);
            wmove(wnd_send_message, 1, 1);

            // обработка строки
            wgetnstr(wnd_send_message, send_and_request.message, 28);
            werase(wnd_send_message);
            start_logo(wnd_see_message, wnd_urers, wnd_send_message);
            wrefresh(wnd_send_message);

            if (0 == strcmp(send_and_request.message, "exit"))
            {
                send_and_request.type_message = 1;
                status = msgsnd(id_message, &send_and_request, sizeof(send_and_request) - sizeof(send_and_request.mtype), 0);
                error_func(status);
                break;
            }
            else if (0 == strcmp(send_and_request.message, "fin"))
            {
                send_and_request.type_message = 6;
                status = msgsnd(id_message, &send_and_request, sizeof(send_and_request) - sizeof(send_and_request.mtype), 0);
                error_func(status);
                break;
            }
            else
            {
                send_and_request.type_message = 3;
                status = msgsnd(id_message, &send_and_request, sizeof(send_and_request) - sizeof(send_and_request.mtype), 0);
                error_func(status);
            }
        }

        pid = wait(&status);
        error_func(pid);

        if (!WIFEXITED(status))
        {
            printf("Дочерний процесс (1) завершился некорректно");
        }

        delwin(wnd_send_message);
        delwin(wnd_urers);
        delwin(wnd_see_message);
        endwin();
    }
    exit(EXIT_SUCCESS);
}

// функция оформления окон
void start_logo(WINDOW *wnd_see_message, WINDOW *wnd_urers, WINDOW *wnd_send_message)
{
    // обводка окон
    box(wnd_send_message, '|', '-');
    box(wnd_urers, '|', '-');
    box(wnd_see_message, '|', '-');

    // оформление окон
    wmove(wnd_see_message, 1, 2);
    wprintw(wnd_see_message, "Chat");
    wmove(wnd_see_message, 2, 2);
    wprintw(wnd_see_message, "--------------------------------------");
    wmove(wnd_urers, 1, 2);
    wprintw(wnd_urers, "Users");
    wmove(wnd_urers, 2, 2);
    wprintw(wnd_urers, "----------------");
}