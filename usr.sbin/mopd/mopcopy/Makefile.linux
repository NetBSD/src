# $NetBSD: Makefile.linux,v 1.1 2016/06/08 01:16:35 christos Exp $
SRCS=mopcopy.c
OBJS=${SRCS:.c=.o}

LDLIBS+=../common/libcommon.a
CFLAGS+=-I../common -DNOAOUT

mopcopy: ${OBJS}

clean:
	rm -f ${OBJS} mopcopy
