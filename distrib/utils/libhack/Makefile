# $NetBSD: Makefile,v 1.5 1999/02/13 02:54:18 lukem Exp $
#
# Stubs to kill off some things from libc:
# This save space on a boot system.
# (See Makefile.inc also.)

LIB=	hack
SRCS=	getgrent.c gethost.c getnet.c getnetgr.c getpwent.c setlocale.c yplib.c

MKPIC=	no
MKPROFILE=no

all: libhack.a

tpwent: getpwent.c
	$(CC) -g -o $@.o -DTEST_MAIN -c getpwent.c
	$(CC) -o $@ $@.o

.include <bsd.lib.mk>
