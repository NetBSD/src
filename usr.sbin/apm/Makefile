#	$NetBSD: Makefile,v 1.5 1997/06/23 14:48:02 mrg Exp $

.if ${MACHINE} == "i386"
SRCS=	apm.c apmsubr.c
#LDADD+=	-lutil
#DPADD+=	${LIBUTIL}

.PATH:	${.CURDIR}/../apmd

CFLAGS+=	-g  -O2 -Wmissing-prototypes -Wall -I${.CURDIR}/../apmd
PROG=	apm
LINKS=          ${BINDIR}/apm ${BINDIR}/zzz
.endif

MAN=	apm.8
MANSUBDIR=/i386
MLINKS=         apm.8 zzz.8

.include <bsd.prog.mk>
# DO NOT DELETE THIS LINE -- make depend depends on it.
