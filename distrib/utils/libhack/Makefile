# $NetBSD: Makefile,v 1.6 1999/03/13 19:08:43 sommerfe Exp $
#
# Stubs to kill off some things from libc:
# This save space on a boot system.
# (See Makefile.inc also.)

LIB=	hack
SRCS=	getcap.c getgrent.c gethost.c getnet.c getnetgr.c getpwent.c setlocale.c yplib.c
WARNS=	1
MKPIC=	no
MKLINT= no
MKPROFILE=no

all: libhack.a

tpwent: getpwent.c
	$(CC) -g -o $@.o -DTEST_MAIN -c getpwent.c
	$(CC) -o $@ $@.o

.include <bsd.lib.mk>
