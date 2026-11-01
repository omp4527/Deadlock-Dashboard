# Multi-stage Docker build for OS Resource Manager Dashboard
FROM alpine:3.19 AS builder
RUN apk add --no-cache g++

WORKDIR /app
COPY backend/ ./backend/

# Compile the native C++17 server with optimization and thread support
RUN g++ -std=c++17 -O3 \
    backend/main.cpp \
    backend/validation.cpp \
    backend/banker.cpp \
    backend/deadlock_detection.cpp \
    backend/prevention_recovery.cpp \
    backend/test_runner.cpp \
    -pthread \
    -o server

# Stage 2: Minimal runtime image (~15MB)
FROM alpine:3.19
RUN apk add --no-cache libstdc++

WORKDIR /app
COPY --from=builder /app/server ./server
COPY frontend/ ./frontend/

ENV PORT=8080
EXPOSE 8080

CMD ["./server"]

