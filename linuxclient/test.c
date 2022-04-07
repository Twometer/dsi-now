#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXRECVSZ 65000
#define HOSTNAME  "10.0.2.140"
#define PORT      34221

int main() {
    struct hostent* host = gethostbyname(HOSTNAME);
    printf("IP: %s\n", inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));

    int sockfd;
    uint8_t buffer[MAXRECVSZ];
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("Failed to create socket\n");
        return 1;
    }

    struct sockaddr_in send_addr;
    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(PORT);
    send_addr.sin_addr.s_addr = *((unsigned long*)(host->h_addr_list[0]));

    struct sockaddr_in recv_addr;
    int recv_addr_len = sizeof(recv_addr);
    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(PORT);
    recv_addr.sin_addr.s_addr = INADDR_ANY;

    const char* request_text =
        "LOGIN DVTP/0.1\n\n";

    bind(sockfd, (struct sockaddr*)&recv_addr, sizeof(recv_addr_len));

    int sent = sendto(sockfd, request_text, strlen(request_text), 0, (struct sockaddr*)&send_addr, sizeof(send_addr));
    printf("Sent %d bytes\n", sent);

    for (;;) {
        int recvd = recvfrom(sockfd, buffer, MAXRECVSZ, 0, (struct sockaddr*)&recv_addr, &recv_addr_len);
        printf("Received %d bytes\n", recvd);
    }

    return 0;
}