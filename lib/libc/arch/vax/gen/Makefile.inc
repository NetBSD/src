#	$NetBSD: Makefile.inc,v 1.13 2004/01/16 10:46:11 ragge Exp $

SRCS+=	byte_swap_2.S byte_swap_4.S bswap64.S \
	fabs.S frexp.c \
	fpclassifyf.c fpclassifyd.c \
	infinityf.c infinity.c infinityl.c isinf.c isinfl.c isnan.c isnanl.c \
	isfinitef.c isfinited.c \
	ldexp.S \
	makecontext.c modf.S \
	resumecontext.c \
	signbitf.c signbitd.c \
	swapcontext.S \
	udiv.S urem.S \
	__setjmp14.S __sigsetjmp14.S _setjmp.S 

.if ${OBJECT_FMT} != "ELF"
SRCS+=	setjmp.S sigsetjmp.S
.endif

SRCS.vax.gen=	Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
LSRCS+=		${SRCS.vax.gen}
DPSRCS+=	${SRCS.vax.gen}
CLEANFILES+=	${SRCS.vax.gen}
