#	$NetBSD: Makefile,v 1.16 2000/09/29 17:47:52 fvdl Exp $

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c

CPPFLAGS += -I$(.CURDIR)/../../libexec/ld.aout_so
.PATH:	$(.CURDIR)/../../libexec/ld.aout_so

MAN=	ldconfig.8

.include <bsd.prog.mk>
