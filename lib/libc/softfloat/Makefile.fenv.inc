#	$NetBSD: Makefile.fenv.inc,v 1.3 2026/10/05 04:43:56 kre Exp $

.PATH:		${.PARSEDIR}

SoftFloat_Makefile_FENV_ParseDir:=${.PARSEDIR}
CPPFLAGS+=	-I${ARCHDIR}/softfloat -I${SoftFloat_Makefile_FENV_ParseDir}
CPPFLAGS+=	-DSOFTFLOAT_FOR_GCC

SRCS+=	fpgetround.c fpsetround.c fpgetmask.c fpsetmask.c \
	fpgetsticky.c fpsetsticky.c

