#	$NetBSD: Makefile,v 1.11 2003/08/01 17:04:04 lukem Exp $

.include <bsd.own.mk>

WARNS?=	2
PROG=		mklocale
SRCS=		yacc.y lex.l
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/lib/libc
YHEADER=	1

.include <bsd.prog.mk>
