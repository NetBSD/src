#	$NetBSD: Makefile,v 1.15 1998/12/17 22:23:44 thorpej Exp $

.include <bsd.own.mk>	# For OBJECT_FMT

.if (${OBJECT_FMT} == "a.out")
PROG=	ldconfig
SRCS=	ldconfig.c shlib.c

CPPFLAGS += -I$(.CURDIR)/../../libexec/ld.aout_so
.PATH:	$(.CURDIR)/../../libexec/ld.aout_so
.endif

MAN=	ldconfig.8

.include <bsd.prog.mk>
