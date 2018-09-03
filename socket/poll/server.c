#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <poll.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define INFTIM -1
#define OPEN_MAX 1024

void do_read_work(int conn_fd)
{
    ssize_t rn;
    char buffer[4096] = {0};

    rn = read(conn_fd, buffer, sizeof(buffer));
    if (rn == -1) {
       fprintf(stderr, "read work failed, %s\n", strerror(errno));
       exit(6);
    }

    printf("Read %ld bytes: %s.\n", rn, buffer);
}

void do_write_work(int conn_fd)
{
    ssize_t wn;
    const char *s = "1234567890";

    int header_size = strlen(s) + 1;
    write(conn_fd, &header_size, sizeof(header_size));

    wn = write(conn_fd, s, header_size);
    if (wn == -1) {
       fprintf(stderr, "write work failed, %s\n", strerror(errno));
       exit(7);
    }
    printf("Write %ld bytes.\n", wn);

}

void do_exection_work(int conn_fd)
{
    /*
    if(close(conn_fd) == -1) {
       fprintf(stderr, "Close socket failed, %s\n", strerror(errno));
       exit(2);
    }
    */
 
    printf("do exection work, fd = %d %s.\n", conn_fd, strerror(errno));
}

int do_accept(int sock_fd)
{
    int conn_fd = -1;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr); 
    bzero(&client_addr, sizeof(client_addr));

    conn_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (conn_fd == -1) {
        fprintf(stderr, "accept failed, %s\n", strerror(errno));
    }

    printf("Received a connect: %d\n", conn_fd);
    return conn_fd;
}

void do_poll_service(int sock_fd)
{
    int pollfds_pos = 1;
    struct pollfd pollfds[OPEN_MAX];

    while(true) {
        int rv;
        pollfds[0].fd = sock_fd;
        pollfds[0].events = POLLRDNORM;

        rv = poll(pollfds, pollfds_pos, INFTIM);

        printf("poll return value = %d\n", rv);
        if (rv == -1) {
            fprintf(stderr, "poll failed, %s\n", strerror(errno));
            exit(1);

        }

        if (pollfds[0].revents & POLLRDNORM) {
            int conn_fd = do_accept(pollfds[0].fd);
            if (conn_fd != -1) {
                pollfds[pollfds_pos].fd = conn_fd;
                pollfds[pollfds_pos++].events = POLLRDNORM | POLLWRNORM;
            }
        }

       for (int i = 1; i < pollfds_pos; i ++) {
           if (pollfds[i].revents & POLLRDNORM) {
               do_read_work(pollfds[i].fd);
           }
           if (pollfds[i].revents & POLLERR) {
               do_exection_work(pollfds[i].fd);
           }
           if (pollfds[i].revents & POLLWRNORM) {
               do_write_work(pollfds[i].fd);
           }
       }

    }
}

int main(int argc, char *argv[])
{
    int sock_fd;
    const char *host_ip = "10.0.2.15";

    if ((sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1) {
       fprintf(stderr, "Create socket failed, %s\n", strerror(errno));
       exit(1);
    }

    int optvalue = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue)) == -1) {
       fprintf(stderr, "set socket option failed, %s\n", strerror(errno));
       exit(4);
    }

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    if (inet_aton(host_ip, &server_addr.sin_addr) == -1) {
       fprintf(stderr, "inet aton failed, %s\n", strerror(errno));
       exit(3);
    }

    if (bind(sock_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
       fprintf(stderr, "bind socket failed, %s\n", strerror(errno));
       exit(4);
    }

    if (listen(sock_fd, 1024) == -1) {
       fprintf(stderr, "listen socket failed, %s\n", strerror(errno));
       exit(4);
    }

    do_poll_service(sock_fd);

    if(close(sock_fd) == -1) {
       fprintf(stderr, "Close socket failed, %s\n", strerror(errno));
       exit(2);
    }
    return 0;
}
