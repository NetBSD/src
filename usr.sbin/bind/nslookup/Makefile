#	$NetBSD: Makefile,v 1.4 1999/11/19 04:20:14 mrg Exp $

.PATH:	${.CURDIR}/../man

WARNS=	0
BINDIR=	/usr/bin
PROG=	nslookup
SRCS=	main.c getinfo.c debug.c send.c skip.c list.c subr.c commands.l
CPPFLAGS+= ${INCLUDE}
DPADD=	${LIBL}
LDADD=	-ll ${LIBRARY}
MAN=	${PROG}.8

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	${PROG}.help
FILESDIR=/usr/share/misc
.endif

.include "../../Makefile.inc"
.include <bsd.prog.mk>
