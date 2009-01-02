#	$NetBSD: Makefile,v 1.12 2009/01/02 00:20:23 tnozaki Exp $

.ifndef HOSTPROG
.include <bsd.own.mk>
.endif

.PATH: ${NETBSDSRCDIR}/lib/libc/locale ${NETBSDSRCDIR}/lib/libc/citrus

PROG=		mklocale
SRCS=		yacc.y lex.l mklocaledb.c fix_grouping.c \
		citrus_bcs_strtoul.c citrus_db_factory.c citrus_db_hash.c
CPPFLAGS+=	-I. -I${.CURDIR} \
		-I${NETBSDSRCDIR}/lib/libc \
		-I${NETBSDSRCDIR}/lib/libc/include \
		-I${NETBSDSRCDIR}/lib/libc/locale \
		-I${NETBSDSRCDIR}/lib/libc/citrus \
		-I${NETBSDSRCDIR}/common/lib/libc/stdlib \
		-DNBCHAR_MAX=127 -DHOSTPROG
YHEADER=	1

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
