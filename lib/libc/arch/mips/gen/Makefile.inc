#	$NetBSD: Makefile.inc,v 1.10 1999/09/29 13:19:16 takemura Exp $

.if  (${MACHINE} == "hpcmips")
SRCS+=	sf_fabs.c frexp.c infinity.c sf_isinf.c sf_ldexp.c sf_modf.c
SRCS+=	sf_flt_rounds.c sf_fpgetmask.c sf_fpgetround.c sf_fpgetsticky.c \
	sf_fpsetmask.c sf_fpsetround.c sf_fpsetsticky.c
.else
SRCS+=	fabs.S frexp.c infinity.c isinf.c isnan.c ldexp.S modf.S
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c
.endif

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S
SRCS+=	byte_swap_2.S byte_swap_4.S bswap64.c

# mips abi builtin extensions (used by GCC for lexical-closure trampoline)
SRCS+=	cacheflush.c

LSRCS+= Lint_bswap16.c Lint_bswap32.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c
