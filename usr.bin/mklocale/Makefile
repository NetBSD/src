#	$NetBSD: Makefile,v 1.8 2002/09/18 14:00:39 lukem Exp $

.include <bsd.own.mk>

WARNS?=	2
PROG=		mklocale
SRCS=		yacc.y lex.l
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/lib/libc/locale
YHEADER=

lex.o:		yacc.h				# for parallel make.

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
