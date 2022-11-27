#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/tcp.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>

#define BUFFER_SIZE 10000
#define NACK "NACK"
#define JOIN "JOIN"
#define QUIT "QUIT"
#define ERR "ERROR"
#define ACK "ACK"

static volatile int sockfd, port;

void get_from_user(char* name, char* p) {
	printf("Please enter %s: ", name);
	fgets(p, BUFFER_SIZE, stdin);
	if ((strlen(p) > 0) && (p[strlen(p) - 1] == '\n')) p[strlen (p) - 1] = '\0';
}
void init_handler() {close(sockfd);}
// void init_socket(char *ip) {
//     server.sin_addr.s_addr = inet_addr(ip);
//     server.sin_port = htons(port);
//     server.sin_family = AF_INET;
//     if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){perror("Socket()");exit(2);} 
//     puts("Socket created successfully");
// 	signal(SIGINT, init_handler);
// 	// if(connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0){perror("connect()");exit(3);}
//     // puts("Connection established");
// }

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "Usage: %s ip_address port\n", argv[0]); exit(1); }
    char group_name[BUFFER_SIZE], buffer[BUFFER_SIZE];
    int port, childpid, joined=1, server_len;
    struct sockaddr_in server;
    port = atoi(argv[2]);
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);
    server.sin_family = AF_INET;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){perror("Socket()");exit(2);} 
    puts("Socket created successfully");
	signal(SIGINT, init_handler);
    // get_from_user("group name", group_name);puts(group_name);
    puts("Sending JOIN request...");
    // JOIN
    server_len = sizeof(server);
	if (sendto(sockfd, JOIN, sizeof(JOIN), 0, (struct sockaddr*)&server, server_len) < 0) {perror("sendto()"); exit(1);}
    // ACK/ERR
    if (read(sockfd, (char *)buffer, BUFFER_SIZE) < 0){perror("recvfrom()"); exit(1);}
    if (strcmp(ACK, buffer) == 0) {puts("Multicast server ACK JOIN request");} 
    else if (strcmp(ERR, buffer) == 0) {puts("Multicast server did not ACK JOIN request");exit(1);}
    puts("Waiting for messages...");
    // Message
    if ((childpid = fork()) == 0) {
        while(joined) {
            bzero(buffer, BUFFER_SIZE);
            if (recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr*)NULL, NULL) < 0){perror("recvfrom()"); exit(1);}
            printf("Message: %s\n", buffer);
            if (strcmp("CLEARALL", buffer) == 0){puts("The client was forced to quit multicast group");joined=0;exit(0);break;}
        }
    }
    // User input
    while(joined) {
        char p[BUFFER_SIZE];
        puts("Press q and enter at anytime to send a QUIT request to server: ");
    	fgets(p, BUFFER_SIZE, stdin);
    	if ((strlen(p) > 0) && (p[strlen(p) - 1] == '\n')) p[strlen (p) - 1] = '\0';
        if (strcmp("q", p) == 0) {
        	if (sendto(sockfd, QUIT, sizeof(QUIT), 0, (struct sockaddr*)&server, sizeof(server)) < 0) {perror("Send()"); exit(1);}
            joined = 0;
            break;
        }
    }
    puts("Exiting...");
    close(sockfd);
    exit(0);
    return 0;
}
