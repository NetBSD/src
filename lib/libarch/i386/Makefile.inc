#	$NetBSD: Makefile.inc,v 1.17 2017/08/12 08:21:30 maxv Exp $

.if (${MACHINE_ARCH} == "i386" || ${MLIBDIR:Unone} == "i386")
SRCS+=	i386_get_ldt.c i386_set_ldt.c \
	i386_iopl.c i386_get_ioperm.c i386_set_ioperm.c \
	i386_mtrr.c
.endif

MAN+=	i386_get_ldt.2 \
	i386_get_mtrr.2 \
	i386_iopl.2

MLINKS+=i386_get_ldt.2 i386_set_ldt.2 \
	i386_get_mtrr.2 i386_set_mtrr.2
