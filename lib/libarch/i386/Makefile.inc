#	$NetBSD: Makefile.inc,v 1.8 1999/08/31 13:50:47 simonb Exp $

.if (${MACHINE_ARCH} == "i386")
SRCS+=	i386_get_ldt.c i386_set_ldt.c \
	i386_iopl.c i386_get_ioperm.c i386_set_ioperm.c \
	i386_vm86.c
.endif

MAN+=	i386_get_ldt.2 \
	i386_iopl.2 i386_get_ioperm.2 \
	i386_vm86.2

MLINKS+=i386_get_ldt.2 i386_set_ldt.2 \
	i386_get_ioperm.2 i386_set_ioperm.2
