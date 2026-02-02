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
    if (argc != 3) {
        fprintf(stderr, "arg mun false. correct: deliver <server address> <server port number>\n");
        exit(1);
    }

    int sockfd;         // used for creating socket handler
    struct sockaddr_in servaddr;    // used to store the IP and PORT of server
    struct hostent *he;             // DNS result: used to store the returned information from gethostbyname funtion
    char input[MAX_BUF_LEN], filename[MAX_BUF_LEN], cmd[MAX_BUF_LEN];

    // get server IP address: convert the input domain name to the system-recognizable IP
    if ((he = gethostbyname(argv[1])) == NULL) {    // gethostbyname: look up the DNS 
        perror("gethostbyname");
        exit(1);
    }

    // prompt user enter "ftp <file name>"
    printf("Enter message: ");
    if (fgets(input, sizeof(input), stdin) == NULL) exit(1);    // input: defined buffer arr; sizeof(): limit size; stdin: where to read from
    sscanf(input, "%s %s", cmd, filename);  // stores "ftp" into cmd，stroes "myphoto.jpg" into filename

    // check if file actually exists: cmd must be "ftp" and file must exists
    if (strcmp(cmd, "ftp") != 0 || access(filename, F_OK) != 0) {
        printf("File not found or invalid command. Exiting.\n");
        exit(1);
    }

    // create UDP socket, SOCK_DGRAM means UDP; AF_INET: IPV4; 0: default UDP in given previous parameterss
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr)); // clear the memory first to prevent garbage initially
    servaddr.sin_family = AF_INET;          // confirm to use IPV4
    servaddr.sin_port = htons(atoi(argv[2]));   // htons = Host TO Network Short; convert argv[2] from ACCII to INTEGER by atoi
    memcpy(&servaddr.sin_addr, he->h_addr_list[0], he->h_length);   // copy the IP address from previous DNS (he->h_addr_list[0]) to servaddr

    // timer， record start clock
    clock_t start, end;
    double cpu_time_used;
    start = clock(); 

    // send ftp to server
    // sendto(from who, what content, how long the content, sign bit, to who, address length)
    sendto(sockfd, "ftp", 3, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    // record end clock
    end = clock();

    // wait to receive server feedback
    // recvfrom(who receive, store where, how long, sign bit, sent from who?, sender address length?)
    char buf[MAX_BUF_LEN];
    int numbytes = recvfrom(sockfd, buf, MAX_BUF_LEN - 1, 0, NULL, NULL);
    if (numbytes < 0) {
        perror("recvfrom failed");
        exit(1);
    }
    buf[numbytes] = '\0';   // mannually add \0 at the end to prevent random code

    // calculate RTT
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("RTT = %f seconds\n", cpu_time_used);

    // check if server replied yes
    if (strcmp(buf, "yes") == 0) {
        printf("Received yes. A file transfer can start.\n"); 
    } else {
        exit(1); 
    }

    // end socket
    close(sockfd);
    return 0;
}