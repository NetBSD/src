#	$NetBSD: Makefile,v 1.3.2.4 2002/09/06 08:42:44 jdolecek Exp $

NOMAN=		# defined

PROG=		installboot
SRCS=		installboot.c
BINDIR=		/usr/mdec
WARNS=		2
LDSTATIC=	-static
#MANSUBDIR=	/x68k

DISKLABELDIR=	${.CURDIR}/../../../../../sbin/disklabel

CPPFLAGS+=	-I${DISKLABELDIR}
.PATH:		${DISKLABELDIR}
SRCS+=		dkcksum.c

.include <bsd.prog.mk>
