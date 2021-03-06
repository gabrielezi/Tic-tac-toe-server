#ifdef _WIN32
#include <winsock2.h>
#define socklen_t int
#else
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>


#define BUFFLEN 1024
#define MAXCLIENTS 2

int findemptyuser(int c_sockets[]){
    int i;
    for (i = 0; i <  MAXCLIENTS; i++){
        if (c_sockets[i] == -1){
            return i;
        }
    }
    return -1;
}
void checkIfRunning(int r_len, int c_sockets[])
{
    int j;
    for (j = 0; j < MAXCLIENTS; j++){
        if (r_len <= 0){
            close(c_sockets[j]);
            c_sockets[j] = -1;
        }
    }
}
int main(int argc, char *argv[]){
#ifdef _WIN32
    WSADATA data;
#endif
    unsigned int port;
    unsigned int clientaddrlen;
    int l_socket;
    int c_sockets[MAXCLIENTS];
    fd_set read_set;

    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;

    int maxfd = 0;
    int i;
    int client_id;

    char buffer[BUFFLEN];

    bool running = true;

    if (argc != 2){
        fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
        return -1;
    }

#ifdef _WIN32
    WSAStartup(MAKEWORD(2,2),&data);
#endif

    port = atoi(argv[1]);
    if ((port < 1) || (port > 65535)){
        fprintf(stderr, "ERROR #1: invalid port specified.\n");
        return -1;
    }

    if ((l_socket = socket(AF_INET, SOCK_STREAM,0)) < 0){
        fprintf(stderr, "ERROR #2: cannot create listening socket.\n");
        return -1;
    }

    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind (l_socket, (struct sockaddr *)&servaddr,sizeof(servaddr))<0){
        fprintf(stderr,"ERROR #3: bind listening socket.\n");
        return -1;
    }

    if (listen(l_socket, 5) <0){
        fprintf(stderr,"ERROR #4: error in listen().\n");
        return -1;
    }

    for (i = 0; i < MAXCLIENTS; i++){
        c_sockets[i] = -1;
    }

    FD_ZERO(&read_set);
    int r_len;


    while (running){

        for (i = 0; i < MAXCLIENTS; i++){
            if (c_sockets[i] != -1){
                FD_SET(c_sockets[i], &read_set);
                if (c_sockets[i] > maxfd){
                    maxfd = c_sockets[i];
                }
            }
        }

        FD_SET(l_socket, &read_set);
        if (l_socket > maxfd){
            maxfd = l_socket;
        }

        select(maxfd+1, &read_set, NULL , NULL, NULL);

        if (FD_ISSET(l_socket, &read_set)){
            client_id = findemptyuser(c_sockets);
            if (client_id != -1){
                clientaddrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, clientaddrlen);
                c_sockets[client_id] = accept(l_socket,
                                              (struct sockaddr*)&clientaddr, &clientaddrlen);
                printf("Connected:  %s, client %d\n",inet_ntoa(clientaddr.sin_addr), client_id);

                if(client_id == 0)
                    strcpy(buffer, "1");
                else if(client_id == 1)
                    strcpy(buffer, "2");
                send(c_sockets[client_id], buffer, BUFFLEN,0);
            }
        }

        for (i = 0; i < MAXCLIENTS; i++){
            if (c_sockets[i] != -1){
                if (FD_ISSET(c_sockets[i], &read_set))
                {
                    memset(&buffer,0,BUFFLEN);
                    r_len = recv(c_sockets[i],buffer,BUFFLEN,0);
                    checkIfRunning(r_len, &c_sockets[i]);
                    printf("Client sent: %s\n", buffer);
                    if(i == 0)
                    {
                        send(c_sockets[i +1 ], buffer, r_len,0);
                    }
                    if(i == 1)
                    {
                        send(c_sockets[i - 1], buffer, r_len, 0);
                    }
                    printf("server sent: %s\n", buffer);
                    memset(&buffer,0,BUFFLEN);
                }
            }
        }
    }

    return 0;
}