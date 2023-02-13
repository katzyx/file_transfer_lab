#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>

#define buff 100

// REFERENCE TAKEN FROM BEEJ'S GUIDE TO NETWORK PROGRAMMING
struct packet{
        unsigned int total_frag;
        unsigned int frag_no;
        unsigned int size;
        char* filename;
        char filedata[1000];
};

int main(int argc, const char* argv[]){
    int sockfd;
    int status;
    int numbytes;
    char recv_buff[100];
    struct addrinfo hints;
    struct addrinfo *servinfo; // points to results
    struct addrinfo *p;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;

    

    if (argc != 3){
        fprintf(stderr, "number of arguments eror\n");
        exit(1);
    }


    memset(&hints, 0, sizeof hints); // make sure struct is empty
    hints.ai_family = AF_INET; // doesn't matter IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM; // UDP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in IP

    status = getaddrinfo(argv[1], argv[2], &hints, &servinfo);
    if (status != 0){
        fprintf(stderr, "getaddrinfo error %s\n", gai_strerror(status));
        exit(1);
    }


    // loop through all results and make a successful socket connection
    for (p = servinfo; p != NULL; p = p->ai_next){
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1){
            perror("client: socket");
            continue;
        }
        break;
        // poggers!
    }

    // not poggers!
    if (p == NULL){
        fprintf(stderr, "client: failed to create socket \n");
        return 2;
    }

    // ask user to input a message
    char command[buff];
    char filename[buff];
    printf("input message: ftp <file name>\n");
    scanf("%s", command);
    scanf("%s", filename);

    if (strcmp(command, "ftp") != 0){
        exit(1);
    }


    // check existence of file
    int exist;
    int bytes_sent, bytes_recv;
    strcat(command, " ");
    strcat(command, filename); 
    char *msg = command;

    // start time
    clock_t rtt = clock();

    if (access(filename, F_OK) != -1){
        // send ftp to server
        bytes_sent = sendto(sockfd, msg, strlen(msg), 0, p->ai_addr, p->ai_addrlen);
        char* msg2 = filename;
        bytes_sent = sendto(sockfd, msg2, strlen(msg2), 0, p->ai_addr, p->ai_addrlen);

    }
    else{
        printf("File \"%s\" does not exist.\n", filename);
        exit(0);
    }
    

    // receive message from server
    addr_len = sizeof their_addr;
    bytes_recv = recvfrom(sockfd, recv_buff, sizeof(recv_buff), 0, (struct sockaddr*) &their_addr, &addr_len);
   
   // end time
   rtt = clock() - rtt;
   printf("RTT: %.3f\n", ((float) rtt * 1000) / CLOCKS_PER_SEC);

 
   
    if (bytes_recv == -1){
        perror("recvfrom");
        exit(1);
    }

    char to_cmp[100];
    to_cmp[0] = 'y';
    to_cmp[1] = 'e';
    to_cmp[2] = 's';
    to_cmp[3] = '\0';

    if (strcmp(recv_buff, to_cmp) == 0){

        printf("A file transfer can start.\n");

    }
    else{
        printf("exit\n");
        exit(0);
    }

    // START FILE TRANSFER:
    // get size of file
    FILE* fp = fopen(filename, "r");

    fseek(fp, 0, SEEK_END); // seek end of file
    int file_size = ftell(fp); // get file pointer
    fseek(fp, 0, SEEK_SET); // seek beginning

    struct packet frag;
    frag.total_frag = (int) (file_size / 1000) + 1;

    // initialize frag number
    frag.frag_no = 1;

    char packet[2048];

    while(1){
        
        frag.size = fread(frag.filedata, sizeof(char), 1000, fp);
        int header_size = sprintf(packet, "%d:%d:%d:%s:", frag.total_frag, frag.frag_no, frag.size, filename);
        memcpy(packet + header_size, frag.filedata, frag.size);

        // printf("\n PACKET packet: %s \n", packet);

        // send header bytes
        int hd_size[1];
        hd_size[0] = header_size;
        bytes_sent = sendto(sockfd, hd_size, sizeof(hd_size), 0, (struct sockaddr*) &their_addr, addr_len);

        // send file
        bytes_sent = sendto(sockfd, packet, header_size + frag.size, 0, (struct sockaddr*) &their_addr, addr_len);

        // acknowledge

        // receive ACK/NACK from server
        strcpy(recv_buff, "");
        addr_len = sizeof their_addr;
        
        bytes_recv = recvfrom(sockfd, recv_buff, sizeof(recv_buff), 0, (struct sockaddr*) &their_addr, &addr_len);
        
        if (bytes_recv == -1){
            perror("recvfrom");
            exit(1);
        }
        if ( recv_buff == "NACK")
        {
            frag.frag_no--;
        }
        
        frag.frag_no++;

        if(frag.frag_no == frag.total_frag + 1)
        {
            break;
        }

    }

    freeaddrinfo(servinfo);
    close(sockfd);
    fclose(fp);

    return 0;
}