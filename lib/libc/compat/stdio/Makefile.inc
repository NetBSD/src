# $NetBSD: Makefile.inc,v 1.1.54.1 2026/11/04 23:19:14 perseant Exp $

.PATH: ${COMPATDIR}/arch/${MACHINE_ARCH:S/m68ksf/m68k/}/stdio ${COMPATDIR}/stdio

SRCS+=		compat_fgetpos.c compat_fsetpos.c
CPPFLAGS.compat_fgetpos.c+=	-I${COMPATDIR}/../stdio
CPPFLAGS.compat_fsetpos.c+=	-I${COMPATDIR}/../stdio
#	$NetBSD: Makefile.inc,v 1.1.54.1 2026/11/04 23:19:14 perseant Exp $

.ifndef ABSTOP
ABSTOP!= cd ${.PARSEDIR}; pwd

.if (${.PARSEDIR} == ${.CURDIR})
RELTOP:= .
.else
RELTOP:= ${.PARSEDIR:S,^${.CURDIR},,}
.endif

# you can set BSD{SRC,OBJ}DIR in Makefile.local.inc
.if exists(${ABSTOP}/Makefile.local.inc)
.include "${ABSTOP}/Makefile.local.inc"
.endif
.endif
