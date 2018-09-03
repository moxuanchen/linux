#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void sleep_nseconds(int num)
{
    for (int i = 0; i < num; i ++) {
        sleep(1);
        printf("sleep %d seconds\n", i);
    }
}

void do_send_and_receive(int conn_fd)
{
    ssize_t rn, wn;
    const char *s = "1234567890";
    char buffer[4096] = {0};

    sleep_nseconds(1);

    wn = write(conn_fd, s, strlen(s));
    if (wn == -1) {
       fprintf(stderr, "write failed, %s\n", strerror(errno));
       exit(7);
    }
    printf("Write %ld bytes.\n", wn);

    int header_size = 0;
    read(conn_fd, &header_size, sizeof(header_size));
    printf("header size = %d\n", header_size);

    rn = read(conn_fd, buffer, header_size);
    if (rn == -1) {
       fprintf(stderr, "read failed, %s\n", strerror(errno));
       exit(6);
    }
    printf("Read %ld bytes:%s.\n", rn, buffer);

}


int main(int argc, char *argv[])
{
    int sock_fd;
    const char *host_ip = "10.0.2.15";

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
       fprintf(stderr, "Create socket failed, %s\n", strerror(errno));
       exit(1);
    }

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    if (inet_aton(host_ip, &server_addr.sin_addr) == -1) {
       fprintf(stderr, "inet aton failed, %s\n", strerror(errno));
       exit(3);
    }

    if (connect(sock_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
       fprintf(stderr, "bind socket failed, %s\n", strerror(errno));
       exit(4);
    }

    while(true) {
        do_send_and_receive(sock_fd);
    }

    if(close(sock_fd) == -1) {
       fprintf(stderr, "Close socket failed, %s\n", strerror(errno));
       exit(2);
    }
    return 0;
}
