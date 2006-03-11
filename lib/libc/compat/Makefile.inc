#	$NetBSD: Makefile.inc,v 1.3 2006/03/11 23:04:03 christos Exp $

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys

COMPATARCHDIR=${COMPATDIR}/arch/${ARCHSUBDIR}
.PATH: ${COMPATARCHDIR}/gen ${COMPATARCHDIR}/sys

.include "${COMPATDIR}/db/Makefile.inc"
.include "${COMPATDIR}/gen/Makefile.inc"
.include "${COMPATDIR}/stdlib/Makefile.inc"
.include "${COMPATDIR}/sys/Makefile.inc"
.include "${COMPATARCHDIR}/Makefile.inc"

