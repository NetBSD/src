#	$NetBSD: Makefile,v 1.6 1998/04/09 00:32:38 tv Exp $

PROG=	lint1
SRCS=	cgram.y scan.l mem1.c mem.c err.c main1.c decl.c tree.c func.c \
	init.c emit.c emit1.c
NOMAN=
LDADD+=	-ll
DPADD+=	${LIBL}
YHEADER=1
CPPFLAGS+=-I.
LINTFLAGS=-aehpz

BINDIR=	/usr/libexec

scan.c: cgram.h

.include <bsd.prog.mk>
