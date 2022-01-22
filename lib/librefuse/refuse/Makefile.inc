# $NetBSD: Makefile.inc,v 1.5 2022/01/22 08:09:40 pho Exp $

.PATH: ${.CURDIR}/refuse

SRCS+=	buf.c
SRCS+=	chan.c
SRCS+=	fs.c
SRCS+=	legacy.c
SRCS+=	poll.c
SRCS+=	session.c
SRCS+=	v11.c
SRCS+=	v21.c
SRCS+=	v22.c
SRCS+=	v25.c
SRCS+=	v26.c
SRCS+=	v30.c
SRCS+=	v32.c

INCS+=	refuse/buf.h
INCS+=	refuse/chan.h
INCS+=	refuse/fs.h
INCS+=	refuse/legacy.h
INCS+=	refuse/poll.h
INCS+=	refuse/session.h
INCS+=	refuse/v11.h
INCS+=	refuse/v21.h
INCS+=	refuse/v22.h
INCS+=	refuse/v23.h
INCS+=	refuse/v25.h
INCS+=	refuse/v26.h
INCS+=	refuse/v28.h
INCS+=	refuse/v29.h
INCS+=	refuse/v30.h
INCS+=	refuse/v32.h
INCS+=	refuse/v34.h
INCS+=	refuse/v35.h
INCS+=	refuse/v38.h
