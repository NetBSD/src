#	$NetBSD: Makefile.inc,v 1.1 2000/05/02 03:47:19 sjg Exp $

.ifndef ABSTOP
ABSTOP!= cd ${.PARSEDIR}; pwd

RELTOP:= ${.PARSEDIR:S,^${.CURDIR},,}

BSDSRCDIR?= ${ABSTOP}

.if exists(${BSDSRCDIR}/Makefile.local.inc)
.include "${BSDSRCDIR}/Makefile.local.inc"
.endif
.endif
