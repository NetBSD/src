#	$NetBSD: Makefile,v 1.2.2.1 2011/02/17 11:59:23 bouyer Exp $
#

.PATH:	${.CURDIR}/../../sys/rump/librump/rumpkern

LIB=		rumpclient
USE_SHLIBDIR=	yes
MAN=		rumpclient.3

.include <bsd.own.mk>

INCS=		rumpclient.h
INCSDIR=	/usr/include/rump

CPPFLAGS+=	-DRUMP_CLIENT -I${.CURDIR} -I${.CURDIR}/../librumpuser
SRCS=		rumpclient.c
SRCS+=		rump_syscalls.c

.include <bsd.lib.mk>
