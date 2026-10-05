#	$NetBSD: Makefile,v 1.8 2026/10/05 04:43:56 kre Exp $
#	@(#)Makefile	8.2 (Berkeley) 2/3/94
#

.include "${.PARSEDIR}/../Makefile.inc"

#LIB=		cold
CompatMakefileParseDir:=${.PARSEDIR}
CPPFLAGS+=	-I${CompatMakefileParseDir}/../include
CPPFLAGS+=	-I${CompatMakefileParseDir}/..
CPPFLAGS+=	-I${CompatMakefileParseDir}/../arch/${ARCHSUBDIR}

.include "${LIBCDIR}/Makefile.inc"

.include "${LIBCDIR}/db/Makefile.inc"
.include "${LIBCDIR}/gen/Makefile.inc"
.include "${LIBCDIR}/locale/Makefile.inc"
.include "${LIBCDIR}/net/Makefile.inc"
.include "${LIBCDIR}/rpc/Makefile.inc"
.include "${LIBCDIR}/stdio/Makefile.inc"
.include "${LIBCDIR}/stdlib/Makefile.inc"
.include "${LIBCDIR}/sys/Makefile.inc"

COMPATARCHDIR=${CompatMakefileParseDir}/arch/${ARCHSUBDIR}
.PATH: ${COMPATARCHDIR}/gen ${COMPATARCHDIR}/sys

.include <bsd.lib.mk>
