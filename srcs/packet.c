#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "packet.h"

int send_packet(int sockfd, PacketType type, const void *payload, uint16_t payload_len) {
    PacketHeader header;
    header.magic = REQ_MAGIC;
    header.version = 1;
    header.type = type;
    header.length = payload_len;

    // 먼저 헤더 전송
    if (send(sockfd, &header, sizeof(PacketHeader), 0) != sizeof(PacketHeader)) {
        perror("send header");
        return -1;
    }

    // 그 다음 payload 전송 (payload가 NULL이면 생략)
    if (payload_len > 0 && payload != NULL) {
        if (send(sockfd, payload, payload_len, 0) != payload_len) {
            perror("send payload");
            return -1;
        }
    }

    return 0;
}

int recv_packet(int sockfd, PacketHeader *out_header, void *payload_buf, size_t buf_size) {
    // 헤더 수신
    ssize_t n = recv(sockfd, out_header, sizeof(PacketHeader), MSG_WAITALL);
    if (n <= 0) {
        perror("recv header");
        return -1;
    }

    // magic number 검사
    if (out_header->magic != REQ_MAGIC && out_header->magic != RES_MAGIC) {
        fprintf(stderr, "Invalid magic number: 0x%x\n", out_header->magic);
        return -1;
    }

    // payload 수신
    if (out_header->length > 0) {
        if (out_header->length > buf_size) {
            fprintf(stderr, "Payload too big!\n");
            return -1;
        }

        n = recv(sockfd, payload_buf, out_header->length, MSG_WAITALL);
        if (n <= 0) {
            perror("recv payload");
            return -1;
        }
    }

    return 0;
}
