# $NetBSD: Makefile.inc,v 1.3 2022/01/22 07:56:16 pho Exp $

.PATH: ${.CURDIR}/refuse

SRCS+=	buf.c
SRCS+=	poll.c
SRCS+=	session.c

INCS+=	refuse/buf.h
INCS+=	refuse/poll.h
INCS+=	refuse/session.h
