#	$NetBSD: Makefile,v 1.3 1997/05/06 20:46:02 gwr Exp $

SRCS=	apm.c apmsubr.c
#LDADD+=	-lutil
#DPADD+=	${LIBUTIL}

.PATH.c: ${.CURDIR}/../apmd

CFLAGS+=	-g  -O2 -Wmissing-prototypes -Wall -I${.CURDIR}/../apmd
PROG=	apm
MAN=	apm.8
MANSUBDIR=/i386
MLINKS=         apm.8 zzz.8
LINKS=          ${BINDIR}/apm ${BINDIR}/zzz

.include <bsd.prog.mk>
# DO NOT DELETE THIS LINE -- make depend depends on it.
