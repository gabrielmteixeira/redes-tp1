#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server port>",  argv[0]);
    printf("example: %s 127.0.0.1 51511", argv[0]);
    exit(EXIT_FAILURE);
}

#define BUFSZ 1024

int main (int argc, char **argv) { //argv[1] stores the address and argv[2] stores the port
    if (argc < 3) {
        usage(argc, argv);    
    }

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) { //socket() returns -1 if an error occurs
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage); // parsing a storage pointer to a sockaddr pointer
    // addr is the server address
    // connect() returns 0 if successful
    // "addr" is a pointer to "storage" 
    if (0 != connect(s, addr, sizeof(storage))) {
        logexit("connect");
    }
    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    
    printf("connected to %s\n", addrstr);

    char buf[BUFSZ]; // buf is the data that will be sent to the server
    memset(buf, 0, BUFSZ);
    printf("mensagem> ");
    fgets(buf, BUFSZ-1, stdin);
    // count stores the number of bytes transmitted
    // strlen(buf) is the size of the string, + 1 aims to include the "\0"
    int count = send(s, buf, strlen(buf) + 1, 0);
    // if count differs from strlen(buf) + 1 (the size of the message sent), an error has ocurred
    if (count != strlen(buf) + 1) {
        logexit("send");
    }

    memset(buf, 0, BUFSZ);
    unsigned total = 0;
    while(1) {
        count = recv(s, buf + total, BUFSZ - total, 0);
        if (count == 0) {
            // Connection terminated
            break;
        }
        total += count;
    }
    close(s);
    
    printf("received %u bytes\n", total);
    puts(buf);

    exit(EXIT_SUCCESS);
}