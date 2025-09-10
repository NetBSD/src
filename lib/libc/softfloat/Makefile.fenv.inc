#	$NetBSD: Makefile.fenv.inc,v 1.1 2025/09/10 23:07:39 nat Exp $

.PATH:		${.CURDIR}/softfloat

CPPFLAGS+=	-I${ARCHDIR}/softfloat -I${.CURDIR}/softfloat
CPPFLAGS+=	-DSOFTFLOAT_FOR_GCC

SRCS+=	fpgetround.c fpsetround.c fpgetmask.c fpsetmask.c \
	fpgetsticky.c fpsetsticky.c

