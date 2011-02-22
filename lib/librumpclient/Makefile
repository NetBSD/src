#	$NetBSD: Makefile,v 1.4 2011/02/22 14:02:54 pooka Exp $
#

.PATH:	${.CURDIR}/../../sys/rump/librump/rumpkern
NOLINT= 	# syscalls are evil

LIB=		rumpclient
USE_SHLIBDIR=	yes
MAN=		rumpclient.3

.include <bsd.own.mk>

INCS=		rumpclient.h
INCSDIR=	/usr/include/rump

CPPFLAGS+=	-DRUMP_CLIENT -I${.CURDIR} -I${.CURDIR}/../librumpuser
SRCS=		rumpclient.c
SRCS+=		rump_syscalls.c

COPTS.rump_syscalls.c+= -fno-strict-aliasing

.include <bsd.lib.mk>
