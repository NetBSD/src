#	$NetBSD: Makefile.virtex.inc,v 1.1.6.2 2007/01/12 01:00:47 ad Exp $

CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403

.ifndef TEXTADDR
BEGIN:
	echo "TEXTADDR not defined in kernel config!"
	exit 1
.endif

SYSTEM_FIRST_OBJ=	virtex_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/virtex/virtex_start.S
