#	$NetBSD: Makefile,v 1.9 2000/07/06 01:11:23 christos Exp $

PROG=	lint1
SRCS=	cgram.y scan.l mem1.c mem.c err.c main1.c decl.c tree.c func.c \
	init.c emit.c emit1.c
MAN=	lint.7
LDADD+=	-ll
DPADD+=	${LIBL}
YHEADER=1
CPPFLAGS+=-I${.CURDIR} -I.
LINTFLAGS=-aehpz

BINDIR=	/usr/libexec

scan.c: cgram.h

lint.7:	${.OBJDIR}/${PROG}
	sh ${.CURDIR}/makeman ${.OBJDIR}/${PROG} -m > ${.TARGET}

.include <bsd.prog.mk>
