#	$NetBSD: Makefile,v 1.13 2002/01/18 21:01:39 thorpej Exp $

PROG=		lint2
SRCS=		main2.c hash.c read.c mem.c mem2.c chk.c msg.c emit.c emit2.c \
		inittyp.c
NOMAN=		# defined
LINTFLAGS=	-abehrz
WFORMAT=1		# hopeless
BINDIR=		/usr/libexec

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif

