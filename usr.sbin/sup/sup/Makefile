#	$NetBSD: Makefile,v 1.1 1997/10/07 01:31:17 thorpej Exp $

PROG=	sup

SRCS=	supcmain.c supcvers.c supcparse.c supcname.c supcmisc.c supcmeat.c

.PATH:	${.CURDIR}/../source

DPADD=	${LIBSUP} ${LIBCRYPT}
LDADD=	${SUPLIB} -lcrypt

sup: .NOPATH

.include <bsd.prog.mk>
