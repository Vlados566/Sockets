#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define N 1024
#define PORT 50100

int main() {
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "Failed to initialize Winsock.\n");
        return 1;
    }

    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        fprintf(stderr, "Failed to create socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address.\n");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Connection failed: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    printf("Connected to the server!\n");

    uint32_t packages_to_receive;
    if (recv(client_socket, (char *)&packages_to_receive, sizeof(packages_to_receive), 0) <= 0) {
        fprintf(stderr, "Failed to receive number of packages: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }
    printf("Receiving %d packages...\n", packages_to_receive);

    FILE *output_file = fopen("../received_video.mp4", "wb");
    if (output_file == NULL) {
        perror("Failed to open output file");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    uint8_t buffer[N];
    int bytes_received;
    for (int i = 0; i < packages_to_receive; i++) {
        bytes_received = recv(client_socket, (char *)buffer, N, 0);
        if (bytes_received <= 0) {
            fprintf(stderr, "Failed to receive data: %d\n", WSAGetLastError());
            break;
        }
        fwrite(buffer, 1, bytes_received, output_file);
        printf("Received package %d/%d (%d bytes)\n", i + 1, packages_to_receive, bytes_received);
    }

    printf("File reception completed.\n");

    fclose(output_file);
    closesocket(client_socket);
    WSACleanup();
    return 0;
}
