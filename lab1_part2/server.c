/* server.c */

#include <stdio.h>      
#include <stdlib.h>     // exit, atoi
#include <string.h>     // strcmp, memset, strlen
#include <unistd.h>     // UNIX std funcs (close, access) 
#include <arpa/inet.h>  // IP address convert funcs (inet_addr, htons)
#include <time.h>       
#include <sys/socket.h> // Socket core funcs (socket, bind, sendto, recvfrom) 
#include <netinet/in.h> // Internet addr family structures (sockaddr_in)
#include <netdb.h>      // Internet db operation (gethostbyname - used for getting IP based on host name)

#define MAX_BUF_LEN 2000

int main(int argc, char *argv[]) {
    // check the parameter
    if (argc != 2) {
        fprintf(stderr, "arg number fault\n");
        exit(1);
    }

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;   // client address： to get the address from client as don't know at the beginning
    char buf[MAX_BUF_LEN];
    socklen_t addr_len;

    // create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("failed creating socket");
        exit(1);
    }

    // fill in the address information of server
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;  // receive as long as the port is currect, no matter network card
    servaddr.sin_port = htons(atoi(argv[1])); // convert port byte order, Host to Network Short

    // bind the socket to the IP just set
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("failed binding");
        close(sockfd);
        exit(1);
    }

    printf("Server listening on port %s...\n", argv[1]);

    // ---------------------------------------------- part 2
    // file pointer, initialized as NULL, means no file opened currently
    FILE *fp = NULL; 

    while (1) {
        addr_len = sizeof(cliaddr);
        // receive data
        // recvfrom(handle, buffer, length, 0, client address, length pointer)
        int numbytes = recvfrom(sockfd, buf, MAX_BUF_LEN - 1, 0, (struct sockaddr *)&cliaddr, &addr_len);
        if (numbytes < 0) continue;
        buf[numbytes] = '\0';

        // determine whether the handshake signal is "ftp"
        if (strcmp(buf, "ftp") == 0) {
            printf("Received handshake: ftp\n");
            sendto(sockfd, "yes", 3, 0, (struct sockaddr *)&cliaddr, addr_len);
            continue; // handshake ends, enter the next loop
        }

        // if is not "ftp", then it must be file data, so begin parsing
        // packet format: total_frag:frag_no:size:filename:DATA

        int total_frag, frag_no, size;
        char filename[100];

        // parse header: find the data before the first colon -> total_frag
        char *ptr = buf;
        total_frag = atoi(ptr);

        // pointer shifts to after the next colon
        ptr = strchr(ptr, ':'); 
        if (ptr == NULL) continue; // format error protection
        ptr++; // skip colon
        frag_no = atoi(ptr);    // parse frag_no

        // then parse size
        ptr = strchr(ptr, ':');
        if (ptr == NULL) continue;
        ptr++;
        size = atoi(ptr);

        // then parse filename
        ptr = strchr(ptr, ':');
        if (ptr == NULL) continue;
        ptr++;
        char *filename_start = ptr;
        ptr = strchr(ptr, ':'); // find the colon after the filename
        if (ptr == NULL) continue;

        // calculate file name length and copy 
        int name_len = ptr - filename_start;
        strncpy(filename, filename_start, name_len);
        filename[name_len] = '\0'; // add the terminator

        // then, skip the last colon, the pointer will pointing to the beginning of the first data
        ptr++;

        // file operation
        printf("Received Packet %d/%d (Size: %d)\n", frag_no, total_frag, size);

        //  if it's the first packet, open file
        if (frag_no == 1) {
            if (fp != NULL) fclose(fp); //  if there's file not closed before, close them first
            fp = fopen(filename, "wb"); // must use wb (binary write)
            if (fp == NULL) {
                perror("fopen failed");
                continue;
            }
        }

        // write into the data
        // ptr: start pointer，size: data length
        if (fp != NULL) {
            fwrite(ptr, 1, size, fp);
        }

        //  if is the last packet, close file
        if (frag_no == total_frag) {
            if (fp != NULL) {
                fclose(fp);
                fp = NULL; // pointer reset
                printf("File %s transfer completed.\n", filename);
            }
        }

        // send back yes as ACK
        sendto(sockfd, "yes", 3, 0, (struct sockaddr *)&cliaddr, addr_len);
    }
}