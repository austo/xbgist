TEST_DIR=$(DVH)/c/xbgist/tests
SOURCE_DIR=$(DVH)/c/xbgist/src

CC = gcc

CFLAGS = -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include \
	-I /usr/local/include -I/$(SOURCE_DIR) -Wall -Wextra -Wno-unused-parameter

LDFLAGS = -lglib-2.0 -luv

SRCS = $(TEST_DIR)/test.c $(SOURCE_DIR)/util.c $(SOURCE_DIR)/xb_types.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all:    test  

clean:
	rm -f out/* *.o core

$(notdir $(OBJS)):	$(SRCS) $(HDRS)
	$(CC) -c -g $(CFLAGS) $(SRCS)

test:	$(notdir $(OBJS))
	$(CC) $^ -o out/$@ $(LDFLAGS)
