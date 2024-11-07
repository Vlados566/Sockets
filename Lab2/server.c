#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define N 1024
#define fileName "../toSend.mp4"
#define PORT 50100

int main() {
    WSADATA wsaData;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "Failed to initialize Winsock.\n");
        return 1;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        fprintf(stderr, "Failed to create socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, 5) == SOCKET_ERROR) {
        fprintf(stderr, "Listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    client_socket = accept(server_socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET) {
        fprintf(stderr, "Accept failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    printf("Client connected!\n");

    FILE *file = fopen(fileName, "rb");
    if (file == NULL) {
        perror("Failed to open file");
        closesocket(client_socket);
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    uint32_t num_packages = (file_size + N - 1) / N;

    send(client_socket, (char *)&num_packages, sizeof(num_packages), 0);

    uint8_t file_part[N];
    int bytes_read;
    for (int i = 0; i < num_packages; i++) {
        bytes_read = fread(file_part, 1, N, file);
        if (bytes_read <= 0) {
            perror("Failed to read from file");
            break;
        }
        if (send(client_socket, (char *)file_part, bytes_read, 0) == SOCKET_ERROR) {
            fprintf(stderr, "Failed to send data: %d\n", WSAGetLastError());
            break;
        }
        printf("Sent package %d/%d (%d bytes)\n", i + 1, num_packages, bytes_read);
    }

    printf("File transmission completed.\n");

    fclose(file);
    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
