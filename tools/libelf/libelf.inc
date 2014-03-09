#	$NetBSD: libelf.inc,v 1.2 2014/03/09 17:10:12 christos Exp $

LIBELFSRCDIR=	${.CURDIR}/../../external/bsd/elftoolchain/dist/libelf
LIBELFOBJDIR!=	cd ${.CURDIR}/../libelf && ${PRINTOBJDIR}
