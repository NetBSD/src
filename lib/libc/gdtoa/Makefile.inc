#	$NetBSD: Makefile.inc,v 1.3 2006/02/16 23:26:19 christos Exp $

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

# wrappers
#SRCS+=	_hdtoa.c _ldtoa.c

# XXX no hexadecimal floating-point string conversion for VAX FP yet
.if ${MACHINE_ARCH} != "vax"
SRCS+=	strtord.c
.endif

# XXX Workaround for gcc-3.3.3 optimsation bug on sh5
.if ${MACHINE_CPU} == "sh5"
COPTS.strtod.c=	-O1
.endif
