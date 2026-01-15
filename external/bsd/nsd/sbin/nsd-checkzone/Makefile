# $NetBSD: Makefile,v 1.3 2026/01/15 21:35:19 christos Exp $

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
