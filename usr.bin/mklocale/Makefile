#	$NetBSD: Makefile,v 1.7.2.1 2004/07/23 14:33:14 tron Exp $

WARNS?=	2
PROG=		mklocale
SRCS=		yacc.y lex.l
CPPFLAGS+=	-I. -I${.CURDIR} -I${.CURDIR}/../../lib/libc
YHEADER=

lex.o:		yacc.h				# for parallel make.

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
