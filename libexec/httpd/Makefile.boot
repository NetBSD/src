# $eterna: Makefile.boot,v 1.7 2005/09/27 20:09:20 mrg Exp $
#
# very simple makefile to compile bozohttpd, should work with every make.
# see Makefile for a list of compile options that may be placed in CFLAGS.

CC=	cc
CFLAGS=	-O

GROFF=	groff -Tascii
CRYPTOLIBDIR=	# -L/usr/local/lib
CRYPTOLIBS=	$(CRYPTOLIBDIR) -lcrypto -lssl

FILES=	bozohttpd.c auth-bozo.c cgi-bozo.c content-bozo.c daemon-bozo.c \
	dir-index-bozo.c ssl-bozo.c tilde-luzah-bozo.c

all:
	$(CC) $(CFLAGS) -o bozohttpd $(FILES) $(CRYPTOLIBS)

man:
	$(GROFF) -mandoc bozohttpd.8 > bozohttpd.cat8

clean:
	rm -f bozohttpd bozohttpd.cat8 *.o
