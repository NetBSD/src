#	$NetBSD: Makefile.inc,v 1.1 2008/09/19 21:41:28 christos Exp $

.include <bsd.own.mk>

LIBAMUOBJDIR!=cd ${.CURDIR}/../../lib/libamu && ${PRINTOBJDIR}

.include "${.CURDIR}/../../Makefile.inc"
