#	$NetBSD: Makefile,v 1.2 2014/07/22 20:25:13 alnsn Exp $
#
# Public Domain.
#

.PATH:	${.CURDIR}/../../../../external/bsd/sljit/sljit \
	${.CURDIR}/../../../../external/bsd/sljit/dist/sljit_src

LIB=	rumpkern_sljit

SRCS=	sljitLir.c sljit_mod.c

# NOTE This is not the best place for icache sync routine but only
# sljit uses it at the moment.
# XXX Think about a good hypercall interface (hi, pooka!) and move
# this stuff to rumpuser.
.if !empty(MACHINE_ARCH:Mmips*)
SRCS+=			cache.c
RUMPCOMP_USER_SRCS=	sljit_rump.c
.PATH:			${.CURDIR}/arch/mips

RUMPCOMP_INCS_DIR:=	${.PARSEDIR}
RUMPCOMP_USER_CPPFLAGS=-I${RUMPCOMP_INCS_DIR}
.endif

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
