#	$NetBSD: Makefile,v 1.4.8.1 2002/08/31 14:52:55 gehenna Exp $

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
