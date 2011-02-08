# $Id: Makefile,v 1.1.1.1.2.1 2011/02/08 16:18:31 bouyer Exp $

CFLAGS=-I../include -Wall -Werror -pedantic -fPIC -ggdb -D_GNU_SOURCE
LIBSASLC_OBJS=\
	dict.o error.o mech_anonymous.o mech_digestmd5.o mech_gssapi.o mech.o parser.o \
	xsess.o mech_crammd5.o mech_external.o mech_login.o mech_plain.o saslc.o crypto.o
LDFLAGS=-lssl -lgssapi -ggdb

all: libsaslc.so

libsaslc.so: $(LIBSASLC_OBJS)
	$(CC) -shared -fPIC -o $@ $(LIBSASLC_OBJS) $(LDFLAGS)

clean:
	rm -f *.o *.so

splint:
	splint -weak -I../include *.[ch]
