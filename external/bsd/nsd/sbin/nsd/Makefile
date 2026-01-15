# $NetBSD: Makefile,v 1.3 2026/01/15 21:35:19 christos Exp $

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
configlexer.c \
zonec.c \
configparser.c

MAN+=${PROG}.conf.5
${PROG}.conf.5: ${PROG}.conf.5.in __subst

COPTS.server.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
