#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib")

#define N 1024
#define FILENAME "../toSend.mp4"
#define PORT 50100

#define REQUEST_NUMBER_OF_PACKAGES 0
#define REQUEST_PACKAGES 1
#define ACKNOWLEDGMENT 2

int main() {
    WSADATA wsaData;
    SOCKET server_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);
    uint32_t num_packages = 0;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "Failed to initialize Winsock.\n");
        return 1;
    }

    server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

    printf("Server listening on port %d...\n", PORT);
    FILE *file = fopen(FILENAME, "rb");
    if (file == NULL) {
        perror("Failed to open file");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    while (1) {
        uint8_t buffer[1];
        int received = recvfrom(server_socket, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (received <= 0) {
            fprintf(stderr, "Failed to receive request: %d\n", WSAGetLastError());
            continue;
        }
        printf("Received request type: %d\n", buffer[0]);

        switch (buffer[0]) {
            case REQUEST_NUMBER_OF_PACKAGES: {
                fseek(file, 0, SEEK_END);
                long file_size = ftell(file);
                rewind(file);

                num_packages = (file_size + N - 1) / N;
                if (sendto(server_socket, (char *)&num_packages, sizeof(num_packages), 0, (struct sockaddr *)&client_addr, client_addr_len) == SOCKET_ERROR) {
                    fprintf(stderr, "Failed to send number of packages: %d\n", WSAGetLastError());
                }
                break;
            }
            case REQUEST_PACKAGES: {
                uint8_t file_part[N];
                uint8_t send_buffer[N + sizeof(uint32_t)];
                uint32_t package_id;

                for (package_id = 1; package_id <= num_packages; package_id++) {
                    int bytes_read = fread(file_part, 1, N, file);
                    if (bytes_read <= 0) {
                        perror("Failed to read from file");
                        break;
                    }

                    memcpy(send_buffer, &package_id, sizeof(package_id));
                    memcpy(send_buffer + sizeof(package_id), file_part, bytes_read);
                    if (sendto(server_socket, (char *)send_buffer, bytes_read + sizeof(package_id), 0, (struct sockaddr *)&client_addr, client_addr_len) == SOCKET_ERROR) {
                        fprintf(stderr, "Failed to send package #%d: %d\n", package_id, WSAGetLastError());
                        break;
                    }

                    printf("Sent package %d (%d bytes)\n", package_id, bytes_read);

                    // Wait for acknowledgment from client
                    uint8_t ack_buffer[1];
                    int ack_received = recvfrom(server_socket, (char *)ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
                    if (ack_received <= 0 || ack_buffer[0] != ACKNOWLEDGMENT) {
                        package_id--;  // Resend if acknowledgment is not received
                        printf("Resending package #%d due to missing acknowledgment\n", package_id);
                    }
                }
                rewind(file);  // Reset file pointer for the next request
                break;
            }
        }
    }

    fclose(file);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
