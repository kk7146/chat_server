# 서버 실행
docker build -t chat-server .
docker run -it --init -p 9000:9000 -p 9001:9001 chat-server

# 클라이언트
nc 127.0.0.1 9000


# 추가한 부분
- 이름 중복 불가하게 만들었음
- list 공간 부족 제한 걸어둠
- 방 인원수 제한
- 방 이름 보여줄 때 방 인원수 제한이랑 현재 인원수 보여줌.