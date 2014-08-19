#	$NetBSD: libelf.inc,v 1.1.12.1 2014/08/20 00:04:54 tls Exp $

LIBELFSRCDIR=	${.CURDIR}/../../external/bsd/elftoolchain/dist/libelf
LIBELFOBJDIR!=	cd ${.CURDIR}/../libelf && ${PRINTOBJDIR}
