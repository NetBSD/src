# $NetBSD: Makefile.inc,v 1.2 2022/01/22 07:54:57 pho Exp $

.PATH: ${.CURDIR}/refuse

SRCS+=	buf.c
SRCS+=	session.c

INCS+=	refuse/buf.h
INCS+=	refuse/session.h
