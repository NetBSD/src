#	$NetBSD: Makefile,v 1.7 2002/09/13 20:40:04 mycroft Exp $

BINDIR=		/usr/sbin
PROG=		catman
SRCS=		catman.c manconf.c
MAN=		catman.8
.PATH:		${.CURDIR}/../../usr.bin/man
CPPFLAGS+=-I${.CURDIR}/../../usr.bin/man

.include <bsd.prog.mk>
