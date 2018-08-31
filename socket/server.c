#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


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

void do_select_service(int sock_fd)
{
    int max_fd = -1;
    fd_set read_set;
    fd_set write_set;
    fd_set exection_set;

    int clients[FD_SETSIZE];


    for (int i = 0; i < FD_SETSIZE; i ++) {
        clients[i] = -1;
    }

    while(true) {
        int rv;

        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_ZERO(&exection_set);
        FD_SET(sock_fd, &read_set);

        for (int i = 0; i < FD_SETSIZE; i ++) {
            if (clients[i] != -1) {
                FD_SET(clients[i], &read_set);
                FD_SET(clients[i], &write_set);
                FD_SET(clients[i], &exection_set);
            }
        }
        max_fd = (max_fd > sock_fd)? max_fd : sock_fd;
        rv = select(max_fd + 1, &read_set, &write_set, &exection_set, NULL);
        printf("select return value = %d\n", rv);
        if (rv == -1) {
            fprintf(stderr, "select failed, %s\n", strerror(errno));
            exit(1);

        }

        if (FD_ISSET(sock_fd, &read_set)) {
            int conn_fd = do_accept(sock_fd);
            max_fd = (max_fd > conn_fd)? max_fd : conn_fd;
            if (conn_fd != -1) {
                FD_SET(conn_fd, &read_set);
                FD_SET(conn_fd, &write_set);
                FD_SET(conn_fd, &exection_set);
                for (int i = 0; i < FD_SETSIZE; i ++) {
                    if (clients[i] == -1) {
                        clients[i] = conn_fd;
                        break;
                    }
                }
            }
            FD_CLR(sock_fd, &read_set);
        }

       for (int i = 0; i < FD_SETSIZE; i ++) {
           int fd;
           if ((fd = clients[i]) == -1) {
               continue;
           }
           if (FD_ISSET(fd, &read_set)) {
               do_read_work(fd);
               FD_CLR(fd, &read_set);
           }
           if (FD_ISSET(fd, &write_set)) {
               do_write_work(fd);
               FD_CLR(fd, &write_set);
           }
           if (FD_ISSET(fd, &exection_set)) {
               do_exection_work(fd);
               FD_CLR(fd, &exection_set);
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

    do_select_service(sock_fd);

    if(close(sock_fd) == -1) {
       fprintf(stderr, "Close socket failed, %s\n", strerror(errno));
       exit(2);
    }
    return 0;
}
