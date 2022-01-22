# $NetBSD: Makefile.inc,v 1.4 2022/01/22 07:57:30 pho Exp $

.PATH: ${.CURDIR}/refuse

SRCS+=	buf.c
SRCS+=	legacy.c
SRCS+=	poll.c
SRCS+=	session.c

INCS+=	refuse/buf.h
INCS+=	refuse/legacy.h
INCS+=	refuse/poll.h
INCS+=	refuse/session.h
