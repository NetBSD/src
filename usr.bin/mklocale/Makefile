#	@(#)Makefile	8.1 (Berkeley) 6/7/93
#	$NetBSD: Makefile,v 1.2 2000/12/22 01:31:48 itojun Exp $
#	$FreeBSD: src/usr.bin/mklocale/Makefile,v 1.19.2.1 1999/08/29 15:31:03 peter Exp $

WARNS?=	1
PROG=	mklocale
SRCS=	yacc.y lex.l #y.tab.h
CPPFLAGS+=-I${.OBJDIR}	# for y.tab.h
CPPFLAGS+=-I${.CURDIR} -I${.CURDIR}/../../include -I${.CURDIR}/../../lib/libc/locale
MAN1=	mklocale.1
CLEANFILES+= y.tab.c y.tab.h
YHEADER=	yes

.include <bsd.prog.mk>
