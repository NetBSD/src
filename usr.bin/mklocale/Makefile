#	$NetBSD: Makefile,v 1.9 2003/03/10 20:41:30 tshiozak Exp $

.include <bsd.own.mk>

WARNS?=	2
PROG=		mklocale
SRCS=		yacc.y lex.l
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/lib/libc
YHEADER=

lex.o:		yacc.h				# for parallel make.

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
