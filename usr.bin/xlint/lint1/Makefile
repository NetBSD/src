#	$NetBSD: Makefile,v 1.13 2001/08/14 10:18:28 tv Exp $

PROG=	lint1
SRCS=	cgram.y scan.l mem1.c mem.c err.c main1.c decl.c tree.c func.c \
	init.c emit.c emit1.c
MAN=	lint.7
LDADD+=	-ll
DPADD+=	${LIBL}
YHEADER=1
CPPFLAGS+=-I${.CURDIR} -I.
LINTFLAGS=-aehpz
WFORMAT=1		#hopeless
BINDIR=	/usr/libexec

scan.c: cgram.h

lint.7:	${PROG}
	sh ${.CURDIR}/makeman ${.OBJDIR}/${PROG} -m > ${.TARGET}

realall: lint.7

CLEANFILES+=${MAN}

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
