# $NetBSD: Makefile,v 1.2.2.1 2026/11/04 22:38:44 perseant Exp $

PROG=nsd-checkzone

SRCS=\
dbaccess.c \
dbcreate.c \
difffile.c \
ipc.c \
mini_event.c \
netio.c \
nsd-checkzone.c \
server.c \
configlexer.c \
zonec.c \
configparser.c

COPTS.server.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
