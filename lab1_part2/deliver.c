/* deliver.c */

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

        // --------------------------------------- section 2: fragments
        // open the file
        FILE *fp = fopen(filename, "rb");
        if (fp == NULL) {
            perror("fopen failed");
            exit(1);
        }

        // calculate the file size
        fseek(fp, 0, SEEK_END);      // pointer move to the end of the file
        long file_size = ftell(fp);  // get current location, means the size of the file
        fseek(fp, 0, SEEK_SET);      // pointer back to the beginning of the file

        // calculate the total fragments number: max 1000 bytes
        int total_frag = file_size / 1000;
        if (file_size % 1000 != 0) total_frag++;

        printf("File size: %ld bytes, Total fragments: %d\n", file_size, total_frag);

        // --------------------------------------- section 2: fragments
        // Packet: "total_frag:frag_no:size:filename:DATA"
        // header is about 100-200 bytes，and data has 1000 bytes，so 2000 should be good
        char packet_buf[2000]; 
        char data_buf[1000];   // only used for the temporarily store the binary data read from file

        // loop start to send every package
        for (int frag_no = 1; frag_no <= total_frag; frag_no++) {
            // read data from file
            // fread(address store the , size of every element, read how much, file pointer), returns the byte number
            int len = fread(data_buf, 1, 1000, fp);
            // compose (Serialize) Header. total_frag:frag_no:size:filename:, no data is put here
            // sprintf returns the written byte numnber, which is the length of the header; sprintf put thses together to packet_buf
            int header_len = sprintf(packet_buf, "%d:%d:%d:%s:", total_frag, frag_no, len, filename);
            // appezd binary data
            // copy the len bytes from data_buf, put it after, the header of packet_buf
            memcpy(packet_buf + header_len, data_buf, len);

            // send. total len = headerlen + datalen
            sendto(sockfd, packet_buf, header_len + len, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
            printf("Packet %d/%d sent (size: %d bytes). Waiting for ACK...\n", frag_no, total_frag, len);

            // Wait for ACK: must receive the confirm then send the next one
            char ack_buf[100];
            int ack_len = recvfrom(sockfd, ack_buf, 99, 0, NULL, NULL);

            // check ack: lab1 assume once receive, then should be correct
            if (ack_len > 0) {
                ack_buf[ack_len] = '\0'; // add \0 to convenient printing
                printf("Received ACK: %s\n", ack_buf);
            } else {
                perror("recvfrom ACK failed");
                exit(1);
            }

            // transfer finished, close file
            printf("File transfer completed.\n");
            fclose(fp); 
        }
    } else {
        exit(1); 
    }

    // end socket
    close(sockfd);
    return 0;
}