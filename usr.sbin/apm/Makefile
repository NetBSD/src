#	$NetBSD: Makefile,v 1.2 1997/03/24 22:02:40 christos Exp $

SRCS=	apm.c apmsubr.c
#LDADD+=	-lutil
#DPADD+=	${LIBUTIL}

.PATH:	${.CURDIR}/../apmd

CFLAGS+=	-g  -O2 -Wmissing-prototypes -Wall -I${.CURDIR}/../apmd
PROG=	apm
MAN=	apm.8
MANSUBDIR=/i386
MLINKS=         apm.8 zzz.8
LINKS=          ${BINDIR}/apm ${BINDIR}/zzz

.include <bsd.prog.mk>
# DO NOT DELETE THIS LINE -- make depend depends on it.
