#	$NetBSD: Makefile,v 1.14 2002/09/18 14:00:44 lukem Exp $

NOMAN=		# defined

PROG=		lint2
SRCS=		main2.c hash.c read.c mem.c mem2.c chk.c msg.c emit.c emit2.c \
		inittyp.c
LINTFLAGS=	-abehrz
WFORMAT=1		# hopeless
BINDIR=		/usr/libexec

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif

