#	$NetBSD: Makefile.inc,v 1.7 1999/06/10 00:49:52 simonb Exp $

.if (${MACHINE_ARCH} == "i386")
.PATH: ${LIBC}/i386

SRCS+=	i386_get_ldt.c i386_set_ldt.c \
	i386_iopl.c i386_get_ioperm.c i386_set_ioperm.c \
	i386_vm86.c

SHLIB_VERSION_FILE= ${.CURDIR}/i386/shlib_version
.endif

MAN+=	i386_get_ldt.2 \
	i386_iopl.2 i386_get_ioperm.2 \
	i386_vm86.2

MLINKS+=i386_get_ldt.2 i386_set_ldt.2 \
	i386_get_ioperm.2 i386_set_ioperm.2
