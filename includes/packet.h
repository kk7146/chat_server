#pragma pack(push, 1) // 구조체 패딩 제거

#define REQ_MAGIC 0x5A5A
#define RES_MAGIC 0xA5A5

typedef enum {
    PACKET_TYPE_MESSAGE = 0,
    PACKET_TYPE_LOGIN,
    PACKET_TYPE_LOGOUT,
    PACKET_TYPE_CREATE_ROOM,
    PACKET_TYPE_JOIN_ROOM,
    PACKET_TYPE_RENAME_ROOM,
    PACKET_TYPE_GRANT_HOST_ROOM,
    PACKET_TYPE_KICK_USER_IN_ROOM,
    PACKET_TYPE_LEAVE_ROOM,
    PACKET_TYPE_REMOVE_ROOM,
    PACKET_TYPE_LIST_ROOMS,
    PACKET_TYPE_LIST_USERS,
    PACKET_TYPE_ERROR,           // 에러 응답용
    PACKET_TYPE_ACK              // 일반 ACK 응답용
} PacketType;

typedef struct {
    uint16_t magic;     // REQ_MAGIC or RES_MAGIC
    uint8_t  version;   // 프로토콜 버전 (예: 1)
    uint8_t  type;      // PacketType
    uint16_t length;    // 데이터(payload) 길이
} PacketHeader;

#pragma pack(pop)