# $NetBSD: Makefile,v 1.1.1.1 1995/10/08 23:08:48 gwr Exp $
#
# Stubs to kill off some things from libc:
# This save space on a boot system.
#

LIB=	hack
SRCS=	getgrent.c gethost.c getnetgr.c getpwent.c

NOPIC=
NOPROFILE=

all: libhack.a

.include <bsd.lib.mk>
