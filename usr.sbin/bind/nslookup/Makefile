#	$NetBSD: Makefile,v 1.3 1999/07/12 08:31:49 tron Exp $

.PATH:	${.CURDIR}/../man

WARNS=	0
BINDIR=	/usr/bin
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
