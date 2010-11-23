#	$NetBSD: Makefile,v 1.2 2010/11/23 12:41:47 pooka Exp $
#

.PATH:	${.CURDIR}/../../sys/rump/librump/rumpkern

LIB=		rumpclient
USE_SHLIBDIR=	yes

.include <bsd.own.mk>

INCS=		rumpclient.h
INCSDIR=	/usr/include/rump

CPPFLAGS+=	-DRUMP_CLIENT -I${.CURDIR} -I${.CURDIR}/../librumpuser
SRCS=		rumpclient.c
SRCS+=		rump_syscalls.c

.include <bsd.lib.mk>
