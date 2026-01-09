#	$NetBSD: Makefile.nintendo.inc,v 1.1 2026/01/09 22:54:27 jmcneill Exp $

CFLAGS+=-mcpu=750
AFLAGS+=-mcpu=750

.ifndef TEXTADDR
BEGIN:
	echo "TEXTADDR not defined in kernel config!"
	exit 1
.endif

SYSTEM_FIRST_OBJ=	locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/${BOARDTYPE}/locore.S
