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


SERVER_SRCS = $(SOURCE_DIR)/server.c $(SOURCE_DIR)/xb_types.c

CLIENT_SRCS = $(CLIENT_DIR)/client.c $(CLIENT_DIR)/xb_client_types.c

COMMON_SRCS = $(SOURCE_DIR)/util.c $(SOURCE_DIR)/sentence_util.c

DEPS_SRCS = $(DEPS_DIR)/tpl.c

TEST_SRCS = $(TEST_DIR)/harness.c


SERVER_OBJS = $(SERVER_SRCS:.c=.o)

CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

COMMON_OBJS = $(COMMON_SRCS:.c=.o)

DEPS_OBJS = $(DEPS_SRCS:.c=.o)

TEST_OBJS = $(TEST_SRCS:.c=.o)


.PHONY: all clean

server:	CFLAGS += -D XBSERVER=1

all:	  server  client  harness

clean:
	rm -f out/* *.o core

$(notdir $(SERVER_OBJS)):	$(SERVER_SRCS)
	$(CC) -c -g $(CFLAGS) $(SERVER_SRCS)

$(notdir $(CLIENT_OBJS)):	$(CLIENT_SRCS)
	$(CC) -c -g $(CFLAGS) $(CLIENT_SRCS)

$(notdir $(DEPS_OBJS)): $(DEPS_SRCS)
	$(CC) -c -g $(CFLAGS) $(DEPS_SRCS)

$(notdir $(COMMON_OBJS)): $(COMMON_SRCS)
	$(CC) -c -g $(CFLAGS) $(COMMON_SRCS)

$(notdir $(TEST_OBJS)): $(TEST_SRCS)
	$(CC) -c -g $(CFLAGS) $(TEST_SRCS)

server:	$(notdir $(SERVER_OBJS)) $(notdir $(COMMON_OBJS)) $(notdir $(DEPS_OBJS))
	$(CC) $^ -o out/$@ $(LDFLAGS)

client: $(notdir $(CLIENT_OBJS)) $(notdir $(COMMON_OBJS)) $(notdir $(DEPS_OBJS))
	$(CC) $^ -o out/$@ $(LDFLAGS)

harness: $(notdir $(TEST_OBJS))
	$(CC) $^ -o out/$@ $(LDFLAGS)
