#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib")

#define N 1024
#define PORT 50100

#define REQUEST_NUMBER_OF_PACKAGES 0
#define REQUEST_PACKAGES 1
#define ACKNOWLEDGMENT 2

int main() {
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    int addr_len = sizeof(server_addr);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "Failed to initialize Winsock.\n");
        return 1;
    }

    client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_socket == INVALID_SOCKET) {
        fprintf(stderr, "Failed to create socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server address.\n");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // Request the number of packages from the server
    uint8_t message = REQUEST_NUMBER_OF_PACKAGES;
    if (sendto(client_socket, (char *)&message, sizeof(message), 0, (struct sockaddr *)&server_addr, addr_len) == SOCKET_ERROR) {
        fprintf(stderr, "Failed to send request: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    uint32_t num_packages;
    if (recvfrom(client_socket, (char *)&num_packages, sizeof(num_packages), 0, (struct sockaddr *)&server_addr, &addr_len) <= 0) {
        fprintf(stderr, "Failed to receive number of packages.\n");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }
    printf("Receiving %d packages...\n", num_packages);

    // Open file in the current directory
    FILE *output_file = fopen("./received_video.mp4", "wb");
    if (output_file == NULL) {
        perror("Failed to open output file");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // Request the file packages
    message = REQUEST_PACKAGES;
    if (sendto(client_socket, (char *)&message, sizeof(message), 0, (struct sockaddr *)&server_addr, addr_len) == SOCKET_ERROR) {
        fprintf(stderr, "Failed to request packages: %d\n", WSAGetLastError());
        fclose(output_file);
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    for (uint32_t i = 1; i <= num_packages; i++) {
        uint8_t buffer[N + sizeof(uint32_t)];
        int bytes_received = recvfrom(client_socket, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &addr_len);
        if (bytes_received <= 0) {
            fprintf(stderr, "Failed to receive data for package #%d: %d\n", i, WSAGetLastError());
            i--;  // Retry receiving this package
            continue;
        }

        uint32_t package_id;
        memcpy(&package_id, buffer, sizeof(package_id));
        fwrite(buffer + sizeof(package_id), 1, bytes_received - sizeof(package_id), output_file);
        printf("Received package #%d (%d bytes)\n", package_id, bytes_received);

        // Send acknowledgment
        uint8_t ack_message = ACKNOWLEDGMENT;
        sendto(client_socket, (char *)&ack_message, sizeof(ack_message), 0, (struct sockaddr *)&server_addr, addr_len);
    }

    printf("File reception completed.\n");
    fclose(output_file);
    closesocket(client_socket);
    WSACleanup();
    return 0;
}
