#	$NetBSD: Makefile,v 1.7 2022/07/31 12:40:35 mlelstv Exp $

.include <bsd.own.mk>

USE_SHLIBDIR=   yes

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
