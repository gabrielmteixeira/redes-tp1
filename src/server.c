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
            char server_response[BUFSZ];
            memset(buf, 0, BUFSZ);
            memset(server_response, 0, BUFSZ);
            size_t count = recv(csock, buf, BUFSZ, 0);

            if (count == 0) {
                close(csock);
                return 1;
            }

            if (verify_exit(buf)) {
                printf("Connection closed\n");
                close(csock);
                exit(EXIT_SUCCESS);
            }
            printf("Buffer: %s\n", buf);

            char filename[BUFSZ];
            char file_content[BUFSZ];
            memset(filename, 0, BUFSZ);
            memset(file_content, 0, BUFSZ);

            char* filename_splitter = strchr(buf, '.'); // used to split the filename from the file content
            if (filename_splitter != NULL) {
                *filename_splitter = '\0'; 
                filename_splitter++;
            }
            strcpy(filename, buf);

            if (strncmp(filename_splitter, "txt", 3) == 0) {
                strcat(filename, ".txt");
            } else if (strncmp(filename_splitter, "c", 1) == 0) {
                strcat(filename, ".c");
            } else if (strncmp(filename_splitter, "cpp", 3) == 0) {
                strcat(filename, ".cpp");
            } else if (strncmp(filename_splitter, "py", 2) == 0) {
                strcat(filename, ".py");
            } else if (strncmp(filename_splitter, "tex", 3) == 0) {
                strcat(filename, ".tex");
            } else if (strncmp(filename_splitter, "java", 4) == 0) {
                strcat(filename, ".java");
            } else {
                logexit("invalid file extension");
            }
            strcpy(file_content, buf + strlen(filename));
            printf("Filename: %s\n", filename);
            printf("File content: %s\n", file_content);

            int file_already_existed = 0;
            if (access(filename, F_OK) == 0) { // verify if file already exists in directory
                file_already_existed = 1;
            }

            FILE *file;
            file = fopen(filename, "w");
            if (file == NULL) {
                logexit("Unable to create/overwrite file");
            }
            fputs(file_content, file);
            fclose(file);

            if (file_already_existed) {
                sprintf(server_response, "file %.500s overwritten\n", filename);
            } else {
                sprintf(server_response, "file %.500s received\n", filename);
            }
            printf("Server response: %s\n", server_response);

            count = send(csock, server_response, strlen(server_response), 0);
            if (count != strlen(server_response)) {
                logexit("send");
            }
        }
        close(csock);
    }

    exit(EXIT_SUCCESS);
}