#	$NetBSD: Makefile,v 1.15 1997/03/30 23:42:24 thorpej Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/17/94

PROG=	savecore

# XXX Temporary, until other ports use new libkvm
.if	(${MACHINE} == "alpha")   || \
	(${MACHINE} == "amiga")   || \
	(${MACHINE} == "atari")   || \
	(${MACHINE} == "pc532")   || \
	(${MACHINE} == "powerpc") || \
	(${MACHINE} == "sparc")   || \
	(${MACHINE} == "sun3")    || \
	(${MACHINE} == "sun3x")
SRCS=	savecore.c
LDADD=	-lkvm
DPADD=	${LIBKVM}
.else
SRCS=	savecore_old.c
.endif
SRCS+=	zopen.c
MAN=	savecore.8
.PATH:	${.CURDIR}/../../usr.bin/compress

.include <bsd.prog.mk>
