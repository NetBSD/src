#	$NetBSD: libelf.inc,v 1.1.6.1 2014/05/22 11:42:31 yamt Exp $

LIBELFSRCDIR=	${.CURDIR}/../../external/bsd/elftoolchain/dist/libelf
LIBELFOBJDIR!=	cd ${.CURDIR}/../libelf && ${PRINTOBJDIR}
