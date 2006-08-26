#	$NetBSD: Makefile.inc,v 1.4 2006/08/26 16:07:01 matt Exp $

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys

COMPATARCHDIR=${COMPATDIR}/arch/${ARCHSUBDIR}
.PATH: ${COMPATARCHDIR}/gen ${COMPATARCHDIR}/sys

.include "${COMPATDIR}/db/Makefile.inc"
.include "${COMPATDIR}/gen/Makefile.inc"
.include "${COMPATDIR}/net/Makefile.inc"
.include "${COMPATDIR}/stdlib/Makefile.inc"
.include "${COMPATDIR}/sys/Makefile.inc"
.include "${COMPATARCHDIR}/Makefile.inc"

