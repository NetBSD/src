#	$NetBSD: Makefile,v 1.14 1998/12/15 22:49:42 pk Exp $

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c

CPPFLAGS += -I$(.CURDIR)/../../libexec/ld.aout_so
.PATH:	$(.CURDIR)/../../libexec/ld.aout_so

MAN=	ldconfig.8

.include <bsd.prog.mk>
