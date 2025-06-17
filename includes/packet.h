#ifndef PACKET_H
#define PACKET_H

#pragma pack(push, 1) // 구조체 패딩 제거

#define REQ_MAGIC 0x5A5A
#define RES_MAGIC 0xA5A5

typedef enum {
    PACKET_TYPE_MESSAGE = 0,
    PACKET_TYPE_LOGIN,
    PACKET_TYPE_LOGOUT, // 이 부분 개발 고민 중..
    PACKET_TYPE_CREATE_ROOM,
    PACKET_TYPE_JOIN_ROOM,
    PACKET_TYPE_RENAME_ROOM,
    PACKET_TYPE_GRANT_HOST_ROOM,
    PACKET_TYPE_KICK_USER_IN_ROOM,
    PACKET_TYPE_LEAVE_ROOM,
    PACKET_TYPE_REMOVE_ROOM,
    PACKET_TYPE_DM,
    PACKET_TYPE_LIST_ROOMS,
    PACKET_TYPE_LIST_USERS,
    PACKET_TYPE_ERROR,              // 에러 응답용인데 패킷 에러에서만 쓰일 듯. 명령어 문법 오류 같은 거에선 안 쓰일 듯?
    PACKET_TYPE_ACK                 // 이 분 고민 중.
} PacketType;

typedef struct {
    uint16_t magic;     // REQ_MAGIC or RES_MAGIC
    uint8_t  version;   // 프로토콜 버전 (예: 1)
    uint8_t  type;      // PacketType
    uint16_t length;    // 데이터(payload) 길이
} PacketHeader;

#pragma pack(pop)

#endif