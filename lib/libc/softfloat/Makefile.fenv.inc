#	$NetBSD: Makefile.fenv.inc,v 1.2 2026/10/04 03:28:45 mrg Exp $

.PATH:		${.PARSEDIR}

CPPFLAGS+=	-I${ARCHDIR}/softfloat -I${.PARSEDIR}
CPPFLAGS+=	-DSOFTFLOAT_FOR_GCC

SRCS+=	fpgetround.c fpsetround.c fpgetmask.c fpsetmask.c \
	fpgetsticky.c fpsetsticky.c

