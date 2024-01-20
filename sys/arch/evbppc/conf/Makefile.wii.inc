#	$NetBSD: Makefile.wii.inc,v 1.1 2024/01/20 21:35:59 jmcneill Exp $

CFLAGS+=-mcpu=750
AFLAGS+=-mcpu=750

.ifndef TEXTADDR
BEGIN:
	echo "TEXTADDR not defined in kernel config!"
	exit 1
.endif

SYSTEM_FIRST_OBJ=	${BOARDTYPE}_locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/${BOARDTYPE}/${BOARDTYPE}_locore.S

SYSTEM_LD_TAIL= \
	mv $@ $@_${TEXTADDR}.elf ; \
	${OBJCOPY} --adjust-vma 0x80000000 $@_${TEXTADDR}.elf $@
