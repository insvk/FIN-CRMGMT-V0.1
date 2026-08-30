CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c17 -I./src
DEBUG_CFLAGS = -Wall -Wextra -g -O0 -std=c17 -I./src
LIBS = -lpthread

WITH_OPENSSL ?= 1
WITH_LIBPQ ?= 1

# OpenSSL / Libpq flags
ifeq ($(WITH_OPENSSL),1)
CFLAGS += -DWITH_OPENSSL
LIBS += -lssl -lcrypto
endif

ifeq ($(WITH_LIBPQ),1)
CFLAGS += -DWITH_LIBPQ
LIBS += -lpq
endif

LIBS += -lm

SRCS = src/main.c src/server.c src/db.c src/auth.c src/tracking_engine.c \
       src/handlers/admin_ops.c src/handlers/customer_ops.c src/handlers/tracking_ops.c \
       src/utils/cJSON.c src/utils/crypto_utils.c

TARGET = bin/crmgmt_server

.PHONY: all release debug clean test

all: release

release:
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

debug:
	mkdir -p bin
	$(CC) $(DEBUG_CFLAGS) $(SRCS) -o $(TARGET)_debug $(LIBS)

clean:
	rm -rf bin

test:
	@echo "Running tests..."
	python3 test_server.py --verify-only
