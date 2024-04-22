#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>

#define QUEUE_NAME "/test_queue"
#define MAX_MSG_SIZE 256

int main() {
    struct mq_attr attr;
    mqd_t mq;
    char buffer[MAX_MSG_SIZE];
    int msg_len;

    // Set up the message queue attributes
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    // Create the message queue
    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (mq == -1) {
        perror("Server: mq_open");
        exit(1);
    }

    // Send message to client
    if (mq_send(mq, "Hi!", sizeof("Hi!"), 0) == -1) {
        perror("Server: mq_send");
        exit(1);
    }

    printf("Server: Sent 'Hi!' to client.\n");

    // Receive response from client
    msg_len = mq_receive(mq, buffer, MAX_MSG_SIZE, NULL);
    if (msg_len == -1) {
        perror("Server: mq_receive");
        exit(1);
    }

    buffer[msg_len] = '\0'; // Null terminate the string
    printf("Server: Received message from client: %s\n", buffer);

    // Close and unlink the message queue
    mq_close(mq);
    mq_unlink(QUEUE_NAME);

    return 0;
}

