#	$NetBSD: Makefile.inc,v 1.1.4.2 2008/10/19 22:39:28 haad Exp $

.include <bsd.own.mk>

LIBAMUOBJDIR!=cd ${.CURDIR}/../../lib/libamu && ${PRINTOBJDIR}

.include "${.CURDIR}/../../Makefile.inc"
