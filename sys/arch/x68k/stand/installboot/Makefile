#	$NetBSD: Makefile,v 1.5 2002/08/27 08:53:18 lukem Exp $

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
