# $NetBSD: Makefile,v 1.2.6.1 1996/09/13 20:01:56 gwr Exp $
#
# Stubs to kill off some things from libc:
# This save space on a boot system.
#

LIB=	hack
SRCS=	getgrent.c gethost.c getnetgr.c getpwent.c \
	setlocale.c yplib.c

NOPIC=
NOPROFILE=

all: libhack.a

tpwent: getpwent.c
	$(CC) -g -o $@.o -DTEST_MAIN -c getpwent.c
	$(CC) -o $@ $@.o

.include <bsd.lib.mk>
