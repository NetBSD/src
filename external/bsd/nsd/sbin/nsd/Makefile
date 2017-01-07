# $NetBSD: Makefile,v 1.1 2017/01/07 19:51:22 christos Exp $

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
