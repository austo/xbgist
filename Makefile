BASE_DIR=$(DVH)/c/xbgist
TEST_DIR=$(BASE_DIR)/tests
SOURCE_DIR=$(BASE_DIR)/src
DEPS_DIR=$(BASE_DIR)/deps

CC = gcc

CFLAGS = -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include \
	-I/usr/local/include -I$(SOURCE_DIR) -I$(DEPS_DIR) \
	-Wall -Wno-unused-parameter

LDFLAGS = -lglib-2.0 -luv

SRCS = $(SOURCE_DIR)/server.c $(SOURCE_DIR)/util.c $(SOURCE_DIR)/xb_types.c \
	$(DEPS_DIR)/tpl.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all:    server  

clean:
	rm -f out/* *.o core

$(notdir $(OBJS)):	$(SRCS) $(HDRS)
	$(CC) -c -g $(CFLAGS) $(SRCS)

server:	$(notdir $(OBJS))
	$(CC) $^ -o out/$@ $(LDFLAGS)
