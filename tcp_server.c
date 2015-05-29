/*
code customised from 
http://www-01.ibm.com/support/knowledgecenter/ssw_ibm_i_71/rzab6/poll.htm
*/
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


#define SERVER_PORT 8888
#define POLL_SIZE 32

void make_nonblocking(int);

int main(const int argc, const char** argv)
{
    int server_fd = -1; 
    struct sockaddr_in server_addr;
    
    char buffer[80];
    int timeout = 3 * 60 * 1000;
    struct pollfd pfds[POLL_SIZE];
    int nfds = 1;
    int poll_size = 0;
    int ret = -1;
    int i = 0;
    int end_server = 0;
    int close_conn = 0;
    int compress_array = 0;
    int new_fd = -1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    make_nonblocking(server_fd);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("bind done\n");

    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen() failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("listening at port %hu ...\n", SERVER_PORT);


    memset(pfds, 0, sizeof(pfds));
    pfds[0].fd = server_fd;
    pfds[0].events = POLLIN;

    do {
        printf("waiting on poll()...\n");
        ret = poll(pfds, nfds, timeout);
        if (ret < 0) {
            perror("poll() failed");
            break;
        }
        if (!ret) {
            printf("poll() timed out.\n");
            break;
        }

        poll_size = nfds;
        for (i = 0; i < poll_size; ++i) {
            if (!(pfds[i].revents)) continue;

            if (pfds[i].revents != POLLIN) {
                printf("error: revents = %d\n", pfds[i].revents);
                end_server = 1;
                break;
            }
            
            if (pfds[i].fd == server_fd) {
                printf("server socket got data\n");
                while (1) {
                    new_fd = accept(server_fd, NULL, NULL);
                    if (new_fd < 0) {
                        if (errno != EWOULDBLOCK) {
                            perror("error: accept():");
                            end_server = 1;
                        }
                        break;
                    }
                    
                    printf("new incoming connection - %d\n", new_fd);
                    make_nonblocking(new_fd);
                    pfds[nfds].fd = new_fd;
                    pfds[nfds].events = POLLIN;
                    ++nfds;
                }
            } else {
                printf("fd %d is readable\n", pfds[i].fd);
                close_conn = 0;
                while (1) {
                    ssize_t ret_size = recv(pfds[i].fd, buffer, sizeof(buffer), 0);
                    if (ret_size < 0) {
                        if (errno != EWOULDBLOCK) {
                            perror("error: recv()");
                            close_conn = 1;
                        }
                        break;
                    }
                    
                    if (!ret_size) {
                        printf("connection closed\n");
                        close_conn = 1;
                        break;
                    }
                    ssize_t len = ret_size;
                    printf("%ld bytes received\n", (long) len);

                    ret_size = send(pfds[i].fd, buffer, len, 0);
                    if (ret_size < 0) {
                        printf("error: send()");
                        close_conn = 1;
                        break;
                    }
                }

                if (close_conn) {
                    close(pfds[i].fd);
                    pfds[i].fd = -1;
                    compress_array = 1;
                }
            }     // end of else section  poll[i].fd not eq to server_fd
        } // end of for loop over poll_size

        if (compress_array) {
            compress_array = 0;
            for (i=0; i < nfds; ++i) {
                if (pfds[i].fd == -1) {
                    int j = 0;
                    for (j=i; j < (nfds-1); ++j) {
                        pfds[j].fd = pfds[j+1].fd;
                    }
                    --i;
                    --nfds;
                }
            }
        }
    } while (!end_server);

    for (i = 0; i < nfds; ++i) {
        if (pfds[i].fd >= 0) close(pfds[i].fd);
    }

    return 0;
}

void make_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, NULL);
    if (flags == -1) {
        perror("set_nonblocking: fcntl: get");
        flags = 0;
    }
    int ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1) {
        perror("set_nonblocking: fcntl: set");
    }
}
