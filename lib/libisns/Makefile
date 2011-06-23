#	$NetBSD: Makefile,v 1.1.1.1.4.1 2011/06/23 14:18:38 cherry Exp $

.include <bsd.own.mk>

LIB=	isns
SRCS=	isns.c isns_pdu.c isns_socketio.c isns_task.c isns_thread.c
SRCS+=	isns_util.c isns_fileio.c

MAN=	isns.3

INCS=	isns.h isns_defs.h
INCSDIR=/usr/include

WARNS=	4

.include <bsd.lib.mk>

.if ${HAVE_GCC} >= 45
COPTS.isns_pdu.c+=	-fno-strict-aliasing
.endif
