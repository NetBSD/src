#	$NetBSD: Makefile,v 1.6 1999/04/04 16:50:47 dante Exp $

BINDIR=		/usr/sbin
PROG=		catman
SRCS=		config.c catman.c
MAN=		catman.8
.PATH:		${.CURDIR}/../../usr.bin/man
CPPFLAGS+=-I${.CURDIR}/../../usr.bin/man

.include <bsd.prog.mk>
