#	$NetBSD: Makefile,v 1.10 1996/05/08 17:00:10 gwr Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/17/94

PROG=	savecore

# XXX Temporary, until other ports use new libkvm
.if	(${MACHINE} == "atari") || \
	(${MACHINE} == "sun3")
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
