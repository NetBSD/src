#	$NetBSD: Makefile,v 1.5.26.1 2020/04/13 08:03:13 martin Exp $

.include <bsd.own.mk>

LIB=	isns
SRCS=	isns.c isns_pdu.c isns_socketio.c isns_task.c isns_thread.c
SRCS+=	isns_util.c isns_fileio.c

MAN=	isns.3

INCS=	isns.h isns_defs.h
INCSDIR=/usr/include

WARNS?=	5

LIBDPLIBS+=	pthread	${.CURDIR}/../libpthread

COPTS.isns.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.lib.mk>
