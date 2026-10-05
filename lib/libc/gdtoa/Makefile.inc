#	$NetBSD: Makefile.inc,v 1.17 2026/10/05 04:43:56 kre Exp $

# gdtoa sources
.PATH:	${.PARSEDIR}
GDtoAMakefileParseDir:=${.PARSEDIR}
CPPFLAGS+=-I${GDtoAMakefileParseDir} -I${LIBCDIR}/locale

.if ${MACHINE_ARCH} == "vax"
CPPFLAGS+=-DNO_FENV_H
.else
CPPFLAGS+=-DHonor_FLT_ROUNDS
.endif

# machine-dependent directory must provide the following:
# 	arith.h gd_qnan.h
CPPFLAGS+=-I${ARCHDIR}/gdtoa

# machine-dependent Makefile must define names of modules to use for
# float conversions:
# 	strtof(): strtof{,_vaxf}.c
# and for long double conversions on platforms with extended-precision
# format, if supported (strtod.c will define an alias otherwise):
#	strtold(): strtold_p{x,xL,Q}.c
#	helpers:   strtop{x,xL,Q}.c
.include "${ARCHDIR}/gdtoa/Makefile.inc"

# public interfaces
SRCS+=	strtod.c

# private interfaces interfacing to libc
SRCS+=	dtoa.c ldtoa.c hdtoa.c gdtoa.c
LINTFLAGS.ldtoa.c+=	-X 132

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

LINTFLAGS.hdtoa.c+=	-X 132	# conversion may lose accuracy
