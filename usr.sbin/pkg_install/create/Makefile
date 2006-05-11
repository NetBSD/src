# $NetBSD: Makefile,v 1.5 2006/05/11 23:16:30 mrg Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_create
SRCS= main.c perform.c pl.c

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.perform.c+=	-Wno-pointer-sign
.endif
