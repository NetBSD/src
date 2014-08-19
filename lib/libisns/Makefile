#	$NetBSD: Makefile,v 1.4.2.1 2014/08/20 00:02:17 tls Exp $

.include <bsd.own.mk>

LIB=	isns
SRCS=	isns.c isns_pdu.c isns_socketio.c isns_task.c isns_thread.c
SRCS+=	isns_util.c isns_fileio.c

MAN=	isns.3

INCS=	isns.h isns_defs.h
INCSDIR=/usr/include

WARNS?=	5

LIBDPLIBS+=	pthread	${.CURDIR}/../libpthread

.include <bsd.lib.mk>
