#	@(#)Makefile	8.1 (Berkeley) 6/6/93
# $FreeBSD: head/usr.bin/sed/Makefile 265420 2014-05-06 04:22:01Z imp $

.include <src.opts.mk>

PROG=	sed
SRCS=	compile.c main.c misc.c process.c

WARNS?=	2

.if ${MK_TESTS} != "no"
SUBDIR+= tests
.endif

.include <bsd.prog.mk>
