#	$NetBSD: Makefile,v 1.10 2003/05/18 07:57:36 lukem Exp $

.include <bsd.own.mk>

WARNS?=	2
PROG=		mklocale
SRCS=		yacc.y lex.l
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/lib/libc
YHEADER=

lex.o:		yacc.h				# for parallel make.

.include <bsd.prog.mk>
