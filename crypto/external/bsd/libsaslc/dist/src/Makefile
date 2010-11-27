# $Id: Makefile,v 1.1.1.1 2010/11/27 21:23:59 agc Exp $
CC=gcc
CFLAGS=-I../include -Wall -pedantic -fPIC -ggdb
LIBSASLC_OBJS=\
	dict.o error.o mech_anonymous.o mech_digestmd5.o mech_gssapi.o mech.o parser.o \
	xsess.o mech_crammd5.o mech_external.o mech_login.o mech_plain.o saslc.o crypto.o
LDFLAGS=-lssl -lgssapi -ggdb

all: libsaslc.so

libsaslc.so: $(LIBSASLC_OBJS)
	$(CC) -shared -fPIC -o $@ $(LIBSASLC_OBJS) $(LDFLAGS)

clean:
	rm -f *.o *.so
