#	$NetBSD: Makefile,v 1.1 2003/06/26 06:30:16 tshiozak Exp $

.ifndef HOSTPROG
.include <bsd.own.mk>
.endif

.PATH: ${NETBSDSRCDIR}/lib/libc/citrus

WARNS?=		2
PROG=		mkesdb
SRCS=		yacc.y lex.l \
		citrus_bcs.c citrus_db_factory.c citrus_db_hash.c \
		citrus_lookup_factory.c
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/lib/libc \
		-I${NETBSDSRCDIR}/lib/libc/citrus \
		-DHOSTPROG
YHEADER=
MKMAN=		no

lex.o:		yacc.h				# for parallel make.

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
