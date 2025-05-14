# 도커 서버 만들기
docker build -t chat-server .
docker run -it --init -p 9000:9000 -p 9001:9001 chat-server

# 외부에서 명령어 실행
nc 127.0.0.1 9000
