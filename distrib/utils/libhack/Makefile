# $NetBSD: Makefile,v 1.3 1996/10/09 00:13:36 jtc Exp $
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
