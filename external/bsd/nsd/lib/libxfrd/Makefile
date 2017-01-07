# $NetBSD: Makefile,v 1.1 2017/01/07 19:51:21 christos Exp $

LIBISPRIVATE=yes
.include <bsd.init.mk>

LIB=xfrd
SRCS=\
remote.c \
xfrd.c \
xfrd-disk.c \
xfrd-notify.c \
xfrd-tcp.c

COPTS.xfrd-tcp.c+=-Wno-stack-protector

.include <bsd.lib.mk>
