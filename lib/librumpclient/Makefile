#	$NetBSD: Makefile,v 1.1 2010/11/04 21:01:29 pooka Exp $
#

.PATH:	${.CURDIR}/../../sys/rump/librump/rumpkern

LIB=		rumpclient

INCS=		rumpclient.h
INCSDIR=	/usr/include/rump

CPPFLAGS+=	-DRUMP_CLIENT -I${.CURDIR} -I${.CURDIR}/../librumpuser
SRCS=		rumpclient.c
SRCS+=		rump_syscalls.c

.include <bsd.lib.mk>
