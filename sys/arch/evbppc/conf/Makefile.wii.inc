#	$NetBSD: Makefile.wii.inc,v 1.2.2.2 2024/02/03 11:47:07 martin Exp $

CFLAGS+=-mcpu=750
AFLAGS+=-mcpu=750

.ifndef TEXTADDR
BEGIN:
	echo "TEXTADDR not defined in kernel config!"
	exit 1
.endif

SYSTEM_FIRST_OBJ=	${BOARDTYPE}_locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/${BOARDTYPE}/${BOARDTYPE}_locore.S
