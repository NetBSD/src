#	$NetBSD: Makefile,v 1.7 2001/11/12 20:23:26 tv Exp $

WARNS?=	2
PROG=		mklocale
SRCS=		yacc.y lex.l
CPPFLAGS+=	-I. -I${.CURDIR} -I${.CURDIR}/../../lib/libc/locale
YHEADER=

lex.o:		yacc.h				# for parallel make.

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
