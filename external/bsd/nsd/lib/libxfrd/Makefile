# $NetBSD: Makefile,v 1.2 2026/01/15 21:35:19 christos Exp $

LIBISPRIVATE=yes
.include <bsd.init.mk>

LIB=xfrd
SRCS=\
metrics.c \
remote.c \
xfrd.c \
xfrd-catalog-zones.c \
xfrd-disk.c \
xfrd-notify.c \
xfrd-tcp.c

COPTS.xfrd-tcp.c+=-Wno-stack-protector

.include <bsd.lib.mk>
