#	$NetBSD: Makefile,v 1.3.2.2 2002/01/10 19:50:32 thorpej Exp $

PROG=		installboot.new
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
