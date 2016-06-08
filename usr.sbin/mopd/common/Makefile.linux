# $NetBSD: Makefile.linux,v 1.1 2016/06/08 01:11:49 christos Exp $
SRCS+=cmp.c device.c dl.c file.c get.c log.c loop-linux2.c mopdef.c nma.c
SRCS+=pf-linux2.c print.c put.c rc.c version.c
OBJS+=${SRCS:.c=.o}

CFLAGS+=-DNOAOUT

libcommon.a: ${OBJS}
	ar cr $@ ${OBJS}

version.c: VERSION
	@rm -f $@
	sed -e 's/.*/char version[] = "&";/' VERSION > $@
clean:
	rm -f ${OBJS} libcommon.a version.c
