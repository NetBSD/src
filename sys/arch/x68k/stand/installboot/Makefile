#	$NetBSD: Makefile,v 1.2 2001/12/02 09:58:56 minoura Exp $

PROG=		installboot.new
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
