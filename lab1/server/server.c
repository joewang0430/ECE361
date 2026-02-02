#include <stdio.h>      
#include <stdlib.h>     // exit, atoi
#include <string.h>     // strcmp, memset, strlen
#include <unistd.h>     // UNIX std funcs (close, access) 
#include <arpa/inet.h>  // IP address convert funcs (inet_addr, htons)
#include <time.h>       
#include <sys/socket.h> // Socket core funcs (socket, bind, sendto, recvfrom) 
#include <netinet/in.h> // Internet addr family structures (sockaddr_in)
#include <netdb.h>      // Internet db operation (gethostbyname - used for getting IP based on host name)

#define MAX_BUF_LEN 100

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

    while (1) {
        addr_len = sizeof(cliaddr);
        // receive the server message, blocking receive
        // recvfrom(handle, buffer, length, 0, client address, length pointer)
        int numbytes = recvfrom(sockfd, buf, MAX_BUF_LEN - 1, 0, (struct sockaddr *)&cliaddr, &addr_len);

        // if receive has error, just continue to the next loop
        if (numbytes < 0) continue;

        buf[numbytes] = '\0';

        // reply based on the msg content
        // see whether is begin with "ftp"
        if (strcmp(buf, "ftp") == 0) {
            sendto(sockfd, "yes", 3, 0, (struct sockaddr *)&cliaddr, addr_len);
        } else {
            sendto(sockfd, "no", 2, 0, (struct sockaddr *)&cliaddr, addr_len);
        }
    }
}