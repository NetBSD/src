#	$NetBSD: Makefile,v 1.1 2001/11/24 16:26:56 minoura Exp $

PROG=		installboot
SRCS=		installboot.c
BINDIR=		/usr/mdec
WARNS=		2
LDSTATIC?=	-static
#MANSUBDIR=	/x68k
MKMAN=		no		# XXX

DISKLABELDIR=	${.CURDIR}/../../../../../sbin/disklabel

CPPFLAGS+=	-I${DISKLABELDIR}
.PATH:		${DISKLABELDIR}
SRCS+=		dkcksum.c

.include <bsd.prog.mk>
