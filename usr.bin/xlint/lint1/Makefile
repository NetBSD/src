#	$NetBSD: Makefile,v 1.8 2000/07/03 02:51:44 matt Exp $

PROG=	lint1
SRCS=	cgram.y scan.l mem1.c mem.c err.c main1.c decl.c tree.c func.c \
	init.c emit.c emit1.c
MKMAN=	no
LDADD+=	-ll
DPADD+=	${LIBL}
YHEADER=1
CPPFLAGS+=-I${.CURDIR} -I.
LINTFLAGS=-aehpz

BINDIR=	/usr/libexec

scan.c: cgram.h

.include <bsd.prog.mk>
