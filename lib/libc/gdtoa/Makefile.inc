#	$NetBSD: Makefile.inc,v 1.1 2006/01/25 15:43:01 kleink Exp $

# gdtoa sources
.PATH: ${.CURDIR}/gdtoa

CPPFLAGS+=-I${ARCHDIR}/gdtoa
CPPFLAGS+=-DNO_FENV_H

# public interfaces
SRCS+=	strtod.c

# private interfaces interfacing to libc
SRCS+=	dtoa.c

# local locking implementation
SRCS+=	gdtoa_locks.c

# private interfaces
SRCS+=	dmisc.c \
	gethex.c gmisc.c \
	hd_init.c hexnan.c \
	misc.c \
	smisc.c strtodg.c sum.c \
	ulp.c

# XXX no hexadecimal floating-point string conversion for VAX FP yet
.if ${MACHINE_ARCH} != "vax"
SRCS+=	strtord.c
.endif
