#	$NetBSD: Makefile,v 1.4 1997/03/12 18:28:55 mycroft Exp $

PROG=	lint1
SRCS=	cgram.c scan.c mem1.c mem.c err.c main1.c decl.c tree.c func.c \
	init.c emit.c emit1.c
NOMAN=
LDADD+=	-ll
DPADD+=	${LIBL}
YFLAGS=	-d
CFLAGS+=-I.
LINTFLAGS=-aehpz
CLEANFILES+=y.tab.h cgram.c scan.c

BINDIR=	/usr/libexec

.include <bsd.prog.mk>
