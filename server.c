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

// REFERENCE TAKEN FROM BEEJ'S GUIDE TO NETWORK PROGRAMMING
struct packet{
        unsigned int total_frag;
        unsigned int frag_no;
        unsigned int size;
        char* filename;
        char filedata[1000];
};

void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[]){
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int status;
    int bytes_recv;
    struct sockaddr_storage their_addr;
    char buf[2048];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2){
        printf("wrong number of arguments\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints); // make sure struct is empty
    hints.ai_family = AF_INET; // doesn't matter IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM; // UDP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in IP

    status = getaddrinfo(NULL, argv[1], &hints, &servinfo);
    if (status != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
        
    }

    // loop through all results and make a successful socket connection
    for (p = servinfo; p != NULL; p = p->ai_next){
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1){
            perror("server: socket");
            continue;
        }
        // binds port to local machine
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
        // poggers!
    }

    // not poggers!
    if (p == NULL){
        fprintf(stderr, "server: failed to bind socket \n");
        return 2;
    }

    freeaddrinfo(servinfo);
    printf("server: waiting to recieve\n");

    addr_len = sizeof their_addr;
    bytes_recv = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*) &their_addr, &addr_len);
    if (bytes_recv == -1){
        perror("recvfrom");
        exit(1);
    }

    printf("server: packet received\n");

    char *protocol = strtok(buf, " ");
    char *filename = strtok(NULL, " ");

    // printf("file name: %s\n", filename);


    char to_cmp[100];
    to_cmp[0] = 'f';
    to_cmp[1] = 't';
    to_cmp[2] = 'p';
    to_cmp[3] = '\0';
    

    // check if message = ftp
    int bytes_sent;
    if (strcmp(to_cmp, buf) == 0){
        // send ftp to client

        char* msg = "yes";
        bytes_sent = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr*) &their_addr, addr_len);
    

    }
    else{
        char* msg = "no";
        bytes_sent = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr*) &their_addr, addr_len);
    }

    
    // create file
    FILE *fp = NULL;


    while(1){
        strcpy(buf, "");
        
        // receive header size
        int hd_size[1];
        bytes_recv = recvfrom(sockfd, hd_size, sizeof(hd_size), 0, (struct sockaddr*) &their_addr, &addr_len);
        
        // read packets
        bytes_recv = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*) &their_addr, &addr_len);

        if (bytes_recv == -1){
            perror("recvfrom");
            exit(1);
        }

        // create struct
        struct packet frag;
        frag.total_frag = atoi(strtok(buf, ":"));
        frag.frag_no = atoi(strtok(NULL, ":"));
        frag.size = atoi(strtok(NULL, ":"));
        frag.filename = strtok(NULL, ":");
        // printf("total fragments %d, fragment number %d, fragment size %d, fragment name %s\n", frag.total_frag, frag.frag_no, frag.size, frag.filename);

        memcpy(frag.filedata, buf + hd_size[0], frag.size);


        if(frag.frag_no == 1)
        {
            fp = fopen(frag.filename, "wb+");
        }

        fwrite(frag.filedata, sizeof(char), frag.size, fp);
        // fprintf(fp, "%s", frag.filedata);

        // printf("file data: %s\n", frag.filedata);
        char* msg = "ACK";
        bytes_sent = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr*) &their_addr, addr_len);
        if (bytes_sent == -1){
            perror("sendto");
            exit(1);
        }

        if(frag.frag_no == frag.total_frag)
        {
            break;
        }

    
    }
    
    fclose(fp);
    close(sockfd);
    return 0;
}
