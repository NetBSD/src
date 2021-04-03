#	$NetBSD: Makefile.virtex.inc,v 1.1.154.1 2021/04/03 22:28:24 thorpej Exp $

CFLAGS+=-mcpu=405
AFLAGS+=-mcpu=405

.ifndef TEXTADDR
BEGIN:
	echo "TEXTADDR not defined in kernel config!"
	exit 1
.endif

SYSTEM_FIRST_OBJ=	virtex_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/virtex/virtex_start.S
