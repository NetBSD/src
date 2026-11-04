# $NetBSD: Makefile,v 1.2.2.1 2026/11/04 22:38:44 perseant Exp $

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
