#	$NetBSD: Makefile,v 1.1.1.1 1998/10/05 18:01:59 tron Exp $

.PATH:	${.CURDIR}/../nslookup \
	${.CURDIR}/../man

WARNS=	0
PROG=	nslookup
SRCS=	main.c getinfo.c debug.c send.c skip.c list.c subr.c commands.l
CPPFLAGS+= ${INCLUDE}
DPADD=	${LIBL}
LDADD=	-ll ${LIBRARY}
MAN=	${PROG}.8
FILES=	${PROG}.help
FILESDIR=/usr/share/misc

.include "../../Makefile.inc"
.include <bsd.prog.mk>
