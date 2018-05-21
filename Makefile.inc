#	$NetBSD: Makefile.inc,v 1.4.96.1 2018/05/21 04:35:48 pgoyette Exp $

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
