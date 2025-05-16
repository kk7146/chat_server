# 서버 실행
docker build -t chat-server .
docker run -it --init -p 9000:9000 -p 9001:9001 chat-server

# 클라이언트
nc 127.0.0.1 9000
