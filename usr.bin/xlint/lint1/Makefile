#	$NetBSD: Makefile,v 1.7.2.1 2000/10/19 16:33:18 he Exp $

PROG=	lint1
SRCS=	cgram.y scan.l mem1.c mem.c err.c main1.c decl.c tree.c func.c \
	init.c emit.c emit1.c
MKMAN=	no
LDADD+=	-ll
DPADD+=	${LIBL}
YHEADER=1
CPPFLAGS+=-I.
LINTFLAGS=-aehpz
WFORMAT=1		#hopeless
BINDIR=	/usr/libexec

scan.c: cgram.h

.include <bsd.prog.mk>
