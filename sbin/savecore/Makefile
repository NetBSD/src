#	$NetBSD: Makefile,v 1.22 1998/02/19 07:08:09 thorpej Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/17/94

PROG=	savecore

# XXX Temporary until these ports are able to use libkvm.
.if	(${MACHINE_ARCH} == "mips")
SRCS=	savecore.old
.else
SRCS=	savecore.c
LDADD=	-lkvm
DPADD=	${LIBKVM}
.endif
SRCS+=	zopen.c
MAN=	savecore.8
.PATH:	${.CURDIR}/../../usr.bin/compress

.include <bsd.prog.mk>
