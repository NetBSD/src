#	$NetBSD: Makefile,v 1.3.2.1 2012/04/17 00:05:28 yamt Exp $

.include <bsd.own.mk>

LIB=	isns
SRCS=	isns.c isns_pdu.c isns_socketio.c isns_task.c isns_thread.c
SRCS+=	isns_util.c isns_fileio.c

MAN=	isns.3

INCS=	isns.h isns_defs.h
INCSDIR=/usr/include

WARNS?=	5

.include <bsd.lib.mk>
