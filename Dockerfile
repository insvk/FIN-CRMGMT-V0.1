# Stage 1: Build C binary
FROM alpine:3.19 AS builder
RUN apk add --no-cache gcc musl-dev make postgresql-dev libpq-dev openssl-dev curl-dev
WORKDIR /app
COPY . .
RUN make clean && make release

# Stage 2: Runtime Container (Ultra-lean ~15MB)
FROM alpine:3.19
RUN apk add --no-cache libpq postgresql-client openssl curl ca-certificates
WORKDIR /root/
COPY --from=builder /app/bin/crmgmt_server ./
COPY --from=builder /app/web ./web
EXPOSE 8080
ENV PORT=8080
CMD ["./crmgmt_server"]
