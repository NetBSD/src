#	$NetBSD: Makefile.pmppc.inc,v 1.1.10.1 2007/10/25 22:35:44 bouyer Exp $

.ifndef TEXTADDR
BEGIN:
	echo "TEXTADDR not defined in kernel config!"
	exit 1
.endif

SYSTEM_FIRST_OBJ=	${BOARDTYPE}_locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/${BOARDTYPE}/${BOARDTYPE}_locore.S
