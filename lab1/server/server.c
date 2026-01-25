#include <stdio.h>      
#include <stdlib.h>     // exit, atoi
#include <string.h>     // strcmp, memset, strlen
#include <unistd.h>     // UNIX std funcs (close, access) 
#include <arpa/inet.h>  // IP address convert funcs (inet_addr, htons)
#include <time.h>       
#include <sys/socket.h> // Socket core funcs (socket, bind, sendto, recvfrom) [cite: 97]
#include <netinet/in.h> // Internet addr family structures (sockaddr_in)
#include <netdb.h>      // Internet db operation (gethostbyname - used for getting IP based on host name)

#define MAXBUFLEN 100

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "arg number fault\n");
        exit(1);
    }

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;

    // create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("failed creating socket");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(atoi(argv[1])); // convert port byte order

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("failed biding");
        close(sockfd);
        exit(1);
    }

    printf("Server listening on port %s...\n", argv[1]);

    while (1) {
        addr_len = sizeof(cliaddr);
        // receive the server message
        int numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *)&cliaddr, &addr_len);
        if (numbytes < 0) continue;

        buf[numbytes] = '\0';

        // reply based on the msg content
        if (strcmp(buf, "ftp") == 0) {
            sendto(sockfd, "yes", 3, 0, (struct sockaddr *)&cliaddr, addr_len);
        } else {
            sendto(sockfd, "no", 2, 0, (struct sockaddr *)&cliaddr, addr_len);
        }
    }
}