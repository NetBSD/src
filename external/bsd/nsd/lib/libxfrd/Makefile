# $NetBSD: Makefile,v 1.1.4.2 2017/03/20 06:56:06 pgoyette Exp $

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
