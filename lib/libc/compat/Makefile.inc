#	$NetBSD: Makefile.inc,v 1.5 2009/01/11 02:46:24 christos Exp $

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys

COMPATARCHDIR=${COMPATDIR}/arch/${ARCHSUBDIR}
.PATH: ${COMPATARCHDIR}/gen ${COMPATARCHDIR}/sys

.include "${COMPATDIR}/db/Makefile.inc"
.include "${COMPATDIR}/gen/Makefile.inc"
.include "${COMPATDIR}/net/Makefile.inc"
.include "${COMPATDIR}/rpc/Makefile.inc"
.include "${COMPATDIR}/stdlib/Makefile.inc"
.include "${COMPATDIR}/sys/Makefile.inc"
.include "${COMPATDIR}/time/Makefile.inc"
.include "${COMPATARCHDIR}/Makefile.inc"
