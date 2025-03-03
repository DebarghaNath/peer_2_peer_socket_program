#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

void *reading(void *arg)
{
    int sock_fd = *((int*)arg);
    char buffer[1024];
    while(1)
    {
        memset(buffer, 0, sizeof(buffer));
        int rcv = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
        if (rcv < 0) 
        {
            printf("Client disconnected\n");
            break;
        } 
        else if (rcv == 0) 
        {
            printf("Client disconnected\n");
            break;
        }
        printf("Received message: %s\n", buffer);
        char *message = "Message Received!";
        send(sock_fd, message, strlen(message), 0);
    }
    close(sock_fd);
    return NULL;
}

void *recving(void *arg)
{
    int sock_fd = *((int*)arg);
    struct sockaddr_in clientaddr;
    socklen_t addrlen = sizeof(clientaddr);
    pthread_t client_thread;
    while(1)
    {
        int client_fd = accept(sock_fd, (struct sockaddr *)&clientaddr, &addrlen);
        if (client_fd < 0) 
        {
            perror("Accept failed");
            continue;
        }
        printf("\nConnection established with %s:%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
        if(pthread_create(&client_thread, NULL, reading, (void *)&client_fd) != 0)
        {
            perror("Failed to create client thread");
            close(client_fd);
        }
    }
    return NULL;
}

void *sending(void *arg)
{
    int sock_fd = *((int *)arg);
    char message[1024];
    char input[100]; 
    while (1) 
    {
        int ch;
        printf("Enter 1 to send a message, or 0 to close connection: ");
        if (fgets(input, sizeof(input), stdin) == NULL) 
        {
            continue; 
        }
        if (input[0] == '\n') 
        {
            continue;
        }
        if (sscanf(input, "%d", &ch) != 1) 
        {
            continue; 
        }
        
        if (ch == 0)
        {
            close(sock_fd);
            printf("Connection closed.\n");
            break;
        }
        else if (ch == 1)
        {
            char ipaddress[100];
            int port;
            
            printf("Enter receiver IP address: ");
            scanf("%s", ipaddress);
            printf("Enter receiver port number: ");
            scanf("%d", &port); 
            struct sockaddr_in serveraddr;
            serveraddr.sin_family = AF_INET;
            serveraddr.sin_port = htons(port);
            if (inet_pton(AF_INET, ipaddress, &serveraddr.sin_addr) <= 0) 
            {
                perror("Invalid address or address not supported");
                continue;
            }
            if (connect(sock_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
            {
                perror("Connection failed");
                continue;
            }
            printf("Connected to %s:%d\n", ipaddress, port);
            printf("Enter message to send: ");
            scanf(" %[^\n]", message); 
            if (send(sock_fd, message, strlen(message), 0) < 0)
            {
                perror("Failed to send message");
            }
            else
            {
                printf("Message sent: %s\n", message);
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        perror("input type <executable_file> <port_no>");
        exit(EXIT_FAILURE);
    }

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd < 0)
    {
        perror("socket wasn't created\n");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in my_addr;
    socklen_t addrlen;
    my_addr.sin_port = htons(atoi(argv[1]));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    addrlen = sizeof(my_addr);
    if(bind(sock_fd, (struct sockaddr*)&my_addr, addrlen) < 0)
    {
        perror("can't bind to port");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    
    int optval = 1;
    if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        perror("can't set socketopt");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("peer listening to port %d \n", atoi(argv[1]));

    if(listen(sock_fd, 10) < 0)
    {
        perror("can't listen");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    
    int peer_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(peer_sock_fd < 0)
    {
        perror("Socket creation failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    
    pthread_t sending_thread, recving_thread;
    
    pthread_create(&sending_thread, NULL, sending, (void *)&peer_sock_fd);
    pthread_create(&recving_thread, NULL, recving, (void *)&sock_fd);

    pthread_join(sending_thread, NULL);
    pthread_join(recving_thread, NULL);

    close(sock_fd);
    exit(EXIT_SUCCESS);
}
