#	$NetBSD: Makefile,v 1.1.1.2 2000/11/20 00:16:18 wiz Exp $

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
