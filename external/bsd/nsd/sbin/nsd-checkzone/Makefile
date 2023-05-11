# $NetBSD: Makefile,v 1.2 2023/05/11 16:18:31 christos Exp $

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
zlexer.c \
zonec.c \
zparser.c

COPTS.server.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
