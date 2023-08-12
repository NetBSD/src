# $NetBSD: Makefile,v 1.1.26.1 2023/08/12 12:39:52 martin Exp $

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

COPTS.server.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
