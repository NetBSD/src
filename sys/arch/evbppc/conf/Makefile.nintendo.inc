#	$NetBSD: Makefile.nintendo.inc,v 1.1.2.2 2026/11/04 23:20:34 perseant Exp $

CFLAGS+=-mcpu=750
AFLAGS+=-mcpu=750

.ifndef TEXTADDR
BEGIN:
	echo "TEXTADDR not defined in kernel config!"
	exit 1
.endif

SYSTEM_FIRST_OBJ=	locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/${BOARDTYPE}/locore.S
