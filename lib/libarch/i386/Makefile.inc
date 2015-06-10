#	$NetBSD: Makefile.inc,v 1.14.24.1 2015/06/10 17:16:24 snj Exp $

.if (${MACHINE_ARCH} == "i386" || ${MLIBDIR:Unone} == "i386")
SRCS+=	i386_get_ldt.c i386_set_ldt.c \
	i386_iopl.c i386_get_ioperm.c i386_set_ioperm.c \
	i386_vm86.c i386_pmc.c i386_mtrr.c
.endif

MAN+=	i386_get_ldt.2 \
	i386_get_mtrr.2 \
	i386_iopl.2 \
	i386_pmc_info.2 \
	i386_vm86.2

MLINKS+=i386_get_ldt.2 i386_set_ldt.2 \
	i386_get_mtrr.2 i386_set_mtrr.2 \
	i386_pmc_info.2 i386_pmc_startstop.2 \
	i386_pmc_info.2 i386_pmc_read.2
