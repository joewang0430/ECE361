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
    if (argc != 3) {
        fprintf(stderr, "usage: deliver <server address> <server port number>\n");
        exit(1);
    }

    int sockfd;
    struct sockaddr_in servaddr;
    struct hostent *he;
    char input[MAXBUFLEN], filename[MAXBUFLEN], cmd[MAXBUFLEN];

    // get server IP address
    if ((he = gethostbyname(argv[1])) == NULL) {
        perror("gethostbyname");
        exit(1);
    }

    // prompt user enter "ftp <file name>"
    printf("Enter message: ");
    if (fgets(input, sizeof(input), stdin) == NULL) exit(1);
    sscanf(input, "%s %s", cmd, filename);

    // check if file actually exists
    if (strcmp(cmd, "ftp") != 0 || access(filename, F_OK) != 0) {
        printf("File not found or invalid command. Exiting.\n");
        exit(1);
    }

    // create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    memcpy(&servaddr.sin_addr, he->h_addr_list[0], he->h_length);

    // send ftp to server
    sendto(sockfd, "ftp", 3, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    // receive server feedback
    char buf[MAXBUFLEN];
    int numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, NULL, NULL);
    if (numbytes < 0) {
        perror("recvfrom failed");
        exit(1);
    }
    buf[numbytes] = '\0';

    if (strcmp(buf, "yes") == 0) {
        printf("A file transfer can start.\n"); 
    } else {
        exit(1); 
    }

    close(sockfd);
    return 0;
}