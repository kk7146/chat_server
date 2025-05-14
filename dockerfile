# ===== 빌드 스테이지 =====
FROM ubuntu:22.04 AS build

RUN apt-get update && apt-get install -y \
    gcc \
	g++ \
    make \
	lldb \
	net-tools \
	iproute2

WORKDIR /usr/src/app

COPY . .

RUN make

# ===== 실행 스테이지 =====
FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y \
    gcc \
	g++ \
    make \
	lldb \
	net-tools \
	iproute2

WORKDIR /usr/src/app

COPY --from=build /usr/src/app/chat-server .

CMD [ "./chat-server" ]
