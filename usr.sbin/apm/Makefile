#	$NetBSD: Makefile,v 1.1 1996/08/25 23:40:51 jtk Exp $

SRCS=	apm.c apmsubr.c
#LDADD+=	-lutil

.PATH:	${.CURDIR}/../apmd

CFLAGS+=	-g  -O2 -Wmissing-prototypes -Wall -I${.CURDIR}/../apmd
PROG=	apm
MAN=	apm.8
MANSUBDIR=/i386
MLINKS=         apm.8 zzz.8
LINKS=          ${BINDIR}/apm ${BINDIR}/zzz

.include <bsd.prog.mk>
# DO NOT DELETE THIS LINE -- make depend depends on it.
