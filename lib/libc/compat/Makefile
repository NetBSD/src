#	$NetBSD: Makefile,v 1.7 2026/10/04 03:28:41 mrg Exp $
#	@(#)Makefile	8.2 (Berkeley) 2/3/94
#

.include "${.PARSEDIR}/../Makefile.inc"

#LIB=		cold
CPPFLAGS+=	-I${.PARSEDIR}/../include -I${.PARSEDIR}/..
CPPFLAGS+=	-I${.PARSEDIR}/../arch/${ARCHSUBDIR}

.include "${LIBCDIR}/Makefile.inc"

.include "${LIBCDIR}/db/Makefile.inc"
.include "${LIBCDIR}/gen/Makefile.inc"
.include "${LIBCDIR}/locale/Makefile.inc"
.include "${LIBCDIR}/net/Makefile.inc"
.include "${LIBCDIR}/rpc/Makefile.inc"
.include "${LIBCDIR}/stdio/Makefile.inc"
.include "${LIBCDIR}/stdlib/Makefile.inc"
.include "${LIBCDIR}/sys/Makefile.inc"

COMPATARCHDIR=${.PARSEDIR}/arch/${ARCHSUBDIR}
.PATH: ${COMPATARCHDIR}/gen ${COMPATARCHDIR}/sys

.include <bsd.lib.mk>
