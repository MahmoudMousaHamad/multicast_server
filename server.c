#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include<sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/tcp.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>

#define BUFFER_SIZE 10000
#define NACK "NACK"
#define JOIN "JOIN"
#define QUIT "QUIT"
#define ERR "ERROR"
#define ACK "ACK"

static volatile int sockfd, port, go=1;
char group_name[BUFFER_SIZE], buffer[BUFFER_SIZE];
int max_clients, n_msgs, i, childpid, n_clients, wpid, status;
struct sockaddr_in zero_sock_addr;
struct sockaddr_in clients[1000];
socklen_t addr_size;

void get_from_user(char* name, char* p) {
	printf("Please enter %s: ", name);
	fgets(p, BUFFER_SIZE, stdin);
	if ((strlen(p) > 0) && (p[strlen(p) - 1] == '\n')) p[strlen (p) - 1] = '\0';
}
void init_handler() {close(sockfd);}
void init_socket() {
	struct sockaddr_in server;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port); 
	server.sin_family = AF_INET; 
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){perror("Socket()");exit(2);}
	puts("Server Socket created successfully");
	if (bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) {perror("Bind()");exit(3);}
	puts("Binding was successful");
	signal(SIGINT, init_handler);
}

void *myThreadFun(void *vargp) {
    struct sockaddr_in client;
    char b[BUFFER_SIZE];
    int client_len;
    client_len = sizeof(client);
    while(go){
        bzero(&client, client_len);bzero(b, BUFFER_SIZE);
        if (recvfrom(sockfd, (char *)b, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr*)&client, &client_len) < 0){perror("recvfrom()"); exit(1);}
        if (strcmp(JOIN, b) == 0) {
            if (n_clients < max_clients) {
                printf("Received JOIN request from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                sendto(sockfd, ACK, sizeof(ACK), 0, (struct sockaddr*)&client, client_len);
                for (int j=0; j<max_clients; j++) {
                    if (memcmp(&clients[j], &zero_sock_addr, addr_size) == 0) {
                        clients[j] = client;
                        printf("Client %s:%d\n", inet_ntoa(clients[j].sin_addr), ntohs(clients[j].sin_port));
                        break;
                    }
                }
                n_clients++;
            } else {
                sendto(sockfd, ERR, sizeof(ERR), 0, (struct sockaddr*)&client, client_len);
            }
        } else if (strcmp(QUIT, b) == 0) {
            printf("Received QUIT request from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
            for (int j=0; j<max_clients; j++) {
                if (memcmp(&clients[j], &client, addr_size) == 0) {
                    bzero(&clients[j], addr_size);
                    break;
                }
            }
            n_clients--;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {fprintf(stderr, "Usage: %s port\n", argv[0]); exit(1);}
    pthread_t thread_id;
    port = atoi(argv[1]);
    bzero(&zero_sock_addr, addr_size);
    init_socket();
    // Get group name and max clients from user
    get_from_user("group name", group_name);puts(group_name);
    get_from_user("max clients", buffer);max_clients=atoi(buffer);printf("%d\n", max_clients);bzero(buffer, BUFFER_SIZE);
    for (int j=0; j<max_clients; j++) {memset(&clients[j], 0, addr_size);printf("Client %s:%d\n", inet_ntoa(clients[j].sin_addr), ntohs(clients[j].sin_port));}
    pthread_create(&thread_id, NULL, myThreadFun, NULL);
    get_from_user("number of messages", buffer);n_msgs=atoi(buffer);printf("%d\n", n_msgs);bzero(buffer, BUFFER_SIZE);
    int client_len = sizeof(clients[0]);
    while(i<n_msgs) {
        bzero(buffer, BUFFER_SIZE);
        get_from_user("message", buffer);
        for (int j=0; j<max_clients; j++) {
            printf("Client %s:%d\n", inet_ntoa(clients[j].sin_addr), ntohs(clients[j].sin_port));
            sendto(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&clients[j], client_len);
        }
        i++;
    }
    while ((wpid = wait(&status)) > 0);
    puts("Exiting...");
    close(sockfd);
    exit(0);
    return 0;
}
