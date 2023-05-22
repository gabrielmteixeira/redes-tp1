#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#define BUFSZ 1024

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int verify_exit(char *buf) {
    buf[strcspn(buf, "\\")] = '\0';
    if (strcmp(buf, "exit") == 0) {
        return 1;
    }
    return 0;
}


int main (int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) { //argv[1] stores the address and argv[2] stores the port
        usage(argc, argv);
    }


    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) { //socket() returns -1 if an error occurs
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) { // 10 is the max number of pendent conections 
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    while(1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen); // returns a new socket, which is used to interact with client
        if (csock == -1) {
            logexit("accept");
        }

        char caddrstr[BUFSZ];
        addrtostr(caddr, caddrstr, BUFSZ);
        printf("[log] connection from %s\n", caddrstr);

        while(1) {
            char buf[BUFSZ];
            memset(buf, 0, BUFSZ);
            size_t count = recv(csock, buf, BUFSZ, 0);
            printf("Buffer: %s\n", buf);
            // printf("[msg] %s, %d, bytes: \n%s", caddrstr, (int)count, buf);
            if(verify_exit(buf)) {
                printf("Connection closed\n");
                break;
            }

            sprintf(buf, "Mensagem teste: remote endpoint: %.500s\n", caddrstr);
            count = send(csock, buf, strlen(buf) + 1, 0);
            if (count != strlen(buf) + 1) {
                logexit("send");
            }
        }
        close(csock);
        exit(EXIT_SUCCESS);
    }

    exit(EXIT_SUCCESS);
}