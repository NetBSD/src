# $NetBSD: Makefile,v 1.4 1997/12/12 03:36:38 gwr Exp $
#
# Stubs to kill off some things from libc:
# This save space on a boot system.
# (See Makefile.inc also.)

LIB=	hack
SRCS= getgrent.c gethost.c getnet.c getnetgr.c getpwent.c \
	setlocale.c yplib.c

NOPIC=
NOPROFILE=

all: libhack.a

tpwent: getpwent.c
	$(CC) -g -o $@.o -DTEST_MAIN -c getpwent.c
	$(CC) -o $@ $@.o

.include <bsd.lib.mk>
