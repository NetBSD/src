#	$NetBSD: Makefile,v 1.3.2.3 2002/02/11 20:09:21 jdolecek Exp $

PROG=		installboot
SRCS=		installboot.c
BINDIR=		/usr/mdec
WARNS=		2
LDSTATIC?=	-static
#MANSUBDIR=	/x68k
NOMAN=		# defined

DISKLABELDIR=	${.CURDIR}/../../../../../sbin/disklabel

CPPFLAGS+=	-I${DISKLABELDIR}
.PATH:		${DISKLABELDIR}
SRCS+=		dkcksum.c

.include <bsd.prog.mk>
