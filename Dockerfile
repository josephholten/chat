FROM alpine:latest

RUN apk update
RUN apk add --update alpine-sdk
RUN apk add cmake

WORKDIR /chat

COPY . ./

RUN mkdir build
RUN cmake -B build && cmake --build build

ENTRYPOINT [ "build/chat" ]
