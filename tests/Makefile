SOURCE_DIR=$(DVH)/c/xbgist/tests

CC = gcc

CFLAGS = -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include \
	-Wall -Wextra -Wno-unused-parameter

LDFLAGS = -lglib-2.0

SRCS = $(SOURCE_DIR)/test_iter.c $(SOURCE_DIR)/util.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all:    test_iter  

clean:
	rm -f out/* *.o core

$(notdir $(OBJS)):	$(SRCS) $(HDRS)
	$(CC) -c -g $(CFLAGS) $(SRCS)

test_iter:	test_iter.o util.o
	$(CC) $^ -o out/$@ $(LDFLAGS)
