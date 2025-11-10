## Chat Server & Client
C, pthread, ncurses 기반으로 구현된 멀티스레드 TCP 채팅 프로그램입니다.
여러 클라이언트가 동시에 서버에 접속하여 터미널 UI를 통해 실시간으로 메시지를 주고받을 수 있습니다.

### 주요 기능
멀티스레드 서버 : 여러 클라이언트의 접속을 동시에 처리
TCP 소켓 통신: 실시간 메시지 송수신
ncurses 기반 터미널 UI: 채팅창, 사용자 목록, 입력창 분리
개인 메시지 (귓속말) 및 채팅방 전환 기능
클라이언트 종료 처리 및 서버 알림 메시지 전송

### 구조
* 서버는 클라이언트마다 스레드를 생성하여 메시지를 수신
* 수신된 메시지를 모든 클라이언트에게 브로드캐스트
* 클라이언트는 송신/수신 스레드를 분리하여 동시 I/O 처리
  
### 실행
#### 서버
docker build -t chat-server .
docker run -it --init -p 9000:9000 -p 9001:9001 chat-server

#### 클라이언트
nc 127.0.0.1 9000
또는
https://github.com/kk7146/chat_server_client

# 추가한 부분
- 이름 중복 불가하게 만들었음
- list 공간 부족 제한 걸어둠
- 방 인원수 제한
- 방 이름 보여줄 때 방 인원수 제한이랑 현재 인원수 보여줌.

### 참고 (클라이언트 버전)
GitHub: [kk7146/chat_server_client](https://github.com/kk7146/chat_server_client)
