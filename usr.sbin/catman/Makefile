#	$NetBSD: Makefile,v 1.6.10.1 2002/11/03 19:51:47 he Exp $

BINDIR=		/usr/sbin
PROG=		catman
SRCS=		catman.c manconf.c
MAN=		catman.8
.PATH:		${.CURDIR}/../../usr.bin/man
CPPFLAGS+=-I${.CURDIR}/../../usr.bin/man

.include <bsd.prog.mk>
