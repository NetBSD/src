# $NetBSD: Makefile,v 1.1.28.1 2026/11/04 22:38:44 perseant Exp $

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
