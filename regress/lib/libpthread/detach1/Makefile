#	$NetBSD: Makefile,v 1.1 2005/01/21 11:54:24 yamt Exp $

WARNS=3

PROG=   detach1

LDADD=	-lpthread
CPPFLAGS+=	-I/usr/src/lib/libpthread/arch/i386

NOMAN=

regress:
	./${PROG}

.include <bsd.prog.mk>
