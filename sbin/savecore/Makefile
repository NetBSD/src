#	$NetBSD: Makefile,v 1.19 1997/05/06 20:45:27 gwr Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/17/94

PROG=	savecore

# XXX Temporary, until other ports use new libkvm
.if	(${MACHINE} == "alpha")   || \
	(${MACHINE} == "amiga")   || \
	(${MACHINE} == "atari")   || \
	(${MACHINE} == "hp300")   || \
	(${MACHINE} == "mac68k")  || \
	(${MACHINE} == "mvme68k") || \
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
.PATH.c: ${.CURDIR}/../../usr.bin/compress

.include <bsd.prog.mk>
