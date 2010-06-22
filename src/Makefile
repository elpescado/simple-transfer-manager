include ../Makefile.inc
CC=gcc
REFERENCES=gtk+-2.0 libcrypto libcurl
CFLAGS=-Wall $(UC_CFLAGS) -g -DHAVE_CRYPTO -DSTM_POSIX
#LIBS=-Wall -dynamic `curl-config --libs` -dynamic `pkg-config --libs $(REFERENCES)` -g
#STATIC_LIBS=`curl-config --static-libs`
LIBS=-g -Wall $(UC_LDFLAGS)  $(STATIC_LIBS)

PACKAGE=stm
VERSION=1.0
PKG=$(PACKAGE)-$(VERSION)

SOURCES=\
	main.c \
	glibcurl.c \
	stm.c \
	stm-main-window.c \
	stm-manager.c \
	stm-new-transfer-window.c \
	stm-panel.c \
	stm-transfer.c \
	stm-transfer-window.c

HEADERS=\
	glibcurl.h \
	stm.h \
	stm-main-window.h \
	stm-manager.h \
	stm-new-transfer-window.h \
	stm-panel.h \
	stm-transfer.h \
	stm-transfer-window.h

EXTRA_DIST=\
	Makefile

OBJS=$(SOURCES:.c=.o)

all: stm

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJS): $(HEADERS)

stm: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

clean:
	rm -rf $(OBJS) stm

dist: dist-tar

dist-tar: $(PKG)
	tar cjf $(PKG).tar.bz2 $(PKG)
	rm -rf $(PKG)

dist-zip: $(PKG)
	rm -f $(PKG).zip
	zip -9 -r $(PKG).zip $(PKG)
	rm -rf $(PKG)

$(PKG): $(SOURCES) $(HEADERS) $(EXTRA_DIST)
	mkdir -p $(PKG)
	cp $(SOURCES) $(HEADERS) $(EXTRA_DIST) $(PKG)

