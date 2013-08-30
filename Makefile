BASE_DIR=$(DVH)/c/xbgist
TEST_DIR=$(BASE_DIR)/tests
SOURCE_DIR=$(BASE_DIR)/src
DEPS_DIR=$(BASE_DIR)/deps
CLIENT_DIR=$(SOURCE_DIR)/client

CC = gcc

CFLAGS = -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include \
	-I/usr/local/include -I$(SOURCE_DIR) -I$(DEPS_DIR) \
	-Wall -Wno-unused-parameter

LDFLAGS = -lglib-2.0 -luv

SERVER_SRCS = $(SOURCE_DIR)/server.c $(SOURCE_DIR)/util.c $(SOURCE_DIR)/xb_types.c \
	$(DEPS_DIR)/tpl.c

CLIENT_SRCS = $(CLIENT_DIR)/client.c $(CLIENT_DIR)/xb_client_types.c \
	$(DEPS_DIR)/tpl.c

SERVER_OBJS = $(SERVER_SRCS:.c=.o)

CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

.PHONY: all clean

all:	  server  client

clean:
	rm -f out/* *.o core

$(notdir $(SERVER_OBJS)):	$(SERVER_SRCS) $(HDRS)
	$(CC) -c -g $(CFLAGS) $(SERVER_SRCS)

$(notdir $(CLIENT_OBJS)):	$(CLIENT_SRCS) $(HDRS)
	$(CC) -c -g $(CFLAGS) $(CLIENT_SRCS)

server:	$(notdir $(SERVER_OBJS))
	$(CC) $^ -o out/$@ $(LDFLAGS)

client:	$(notdir $(CLIENT_OBJS))
	$(CC) $^ -o out/$@ $(LDFLAGS)
