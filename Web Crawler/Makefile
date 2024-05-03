CC = gcc
CFLAGS = -std=c11 -pedantic -pthread -I/usr/include/libxml2 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LIBS = -lxml2 -lglib-2.0 -lcurl

all: crawler

crawler: crawler.c
	$(CC) $(CFLAGS) crawler.c -o crawler $(LIBS)

clean:
	rm -f crawler

run: crawler
	./crawler

