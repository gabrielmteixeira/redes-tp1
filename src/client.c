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

    FILE *file;
    char command[100];
    char filename[100];
    char extension[100];
    char allowed_extensions[6][10];

    strcpy(allowed_extensions[0], "txt");
    strcpy(allowed_extensions[1], "c");
    strcpy(allowed_extensions[2], "cpp");
    strcpy(allowed_extensions[3], "py");
    strcpy(allowed_extensions[4], "tex");
    strcpy(allowed_extensions[5], "java");

    printf("Enter command: \n");
    fgets(command, sizeof(command), stdin);

    char *select_substring = strstr(command, "select file");
    if(select_substring != NULL) {

        strcpy(filename, strrchr(command, ' ') + 1); // stores the substring correspondent to the filename in "filename"
        filename[strcspn(filename, "\n")] = '\0'; // removes the '/n' after the filename
        strcpy(extension, strrchr(filename, '.') + 1); // stores the substring correspondent to the file extension in "extension"
        // tests if the extension is allowed
        int extension_is_allowed = 0;
        for(int i = 0; i < 6; i++) {
            if(strcmp(allowed_extensions[i], extension) == 0) {
                printf("%s is allowed\n", extension);
                extension_is_allowed = 1;
                break;
            }
        }

        if (!extension_is_allowed) {
            return -1;
        }

        file = fopen(filename, "r");
        if (file == NULL) {
            printf("Unable to open the file.\n");
            return 1;
        }

        // Determine the size of the file
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Allocate memory to store the file contents
        char *file_content = (char *)malloc(file_size + 1);
        if (file_content == NULL) {
            printf("Memory allocation failed.\n");
            fclose(file);
            return 1;
        }

        // Read the file content
        fread(file_content, 1, file_size, file);
        file_content[file_size] = '\0';

        fclose(file);

        printf("File content:\n%s\n", file_content);

        free(file_content);
    }    

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