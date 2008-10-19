#	$NetBSD: Makefile,v 1.1.4.2 2008/10/19 22:39:28 haad Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	fixmount

DIST=	${IDIST}/fixmount
.PATH:	${DIST} ${IDIST}/conf/checkmount

SRCS=	fixmount.c checkmount_bsd44.c
MAN+=	fixmount.8

CPPFLAGS+=	-I${DIST}
LDADD+=		${LIBAMU} -lrpcsvc
DPADD+=		${LIBRPCSVC}

.include <bsd.prog.mk>
