#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>

#define QUEUE_NAME "/test_queue"
#define MAX_MSG_SIZE 256

int main() {
    mqd_t mq;
    char buffer[MAX_MSG_SIZE];
    int msg_len;

    // Open the message queue
    mq = mq_open(QUEUE_NAME, O_RDWR);
    if (mq == -1) {
        perror("Client: mq_open");
        exit(1);
    }

    // Receive message from server
    msg_len = mq_receive(mq, buffer, MAX_MSG_SIZE, NULL);
    if (msg_len == -1) {
        perror("Client: mq_receive");
        exit(1);
    }

    buffer[msg_len] = '\0'; // Null terminate the string
    printf("Client: Received message from server: %s\n", buffer);

    // Send response to server
    if (mq_send(mq, "Hello!", sizeof("Hello!"), 0) == -1) {
        perror("Client: mq_send");
        exit(1);
    }

    printf("Client: Sent 'Hello!' to server.\n");

    // Close the message queue
    mq_close(mq);

    return 0;
}

