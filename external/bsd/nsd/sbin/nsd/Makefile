# $NetBSD: Makefile,v 1.1.4.2 2017/03/20 06:56:06 pgoyette Exp $

PROG=nsd

SRCS=\
dbaccess.c \
dbcreate.c \
difffile.c \
ipc.c \
mini_event.c \
netio.c \
nsd.c \
server.c \
zlexer.c \
zonec.c \
zparser.c

MAN+=${PROG}.conf.5
${PROG}.conf.5: ${PROG}.conf.5.in __subst

.include <bsd.prog.mk>
