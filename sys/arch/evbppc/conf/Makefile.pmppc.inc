#	$NetBSD: Makefile.pmppc.inc,v 1.2.2.2 2007/10/23 20:35:40 ad Exp $

.ifndef TEXTADDR
BEGIN:
	echo "TEXTADDR not defined in kernel config!"
	exit 1
.endif

SYSTEM_FIRST_OBJ=	${BOARDTYPE}_locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/${BOARDTYPE}/${BOARDTYPE}_locore.S
