#	$NetBSD: Makefile,v 1.3 1997/03/24 22:02:52 christos Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:48 vixie Exp 

.PATH:	${.CURDIR}/../nslookup \
	${.CURDIR}/../man

PROG=	nslookup
SRCS=	main.c getinfo.c debug.c send.c skip.c list.c subr.c commands.l
CFLAGS+= ${INCLUDE}
DPADD=	${LIBL}
LDADD=	-ll
CLEANFILES+=	commands.c lex.yy.c lex.yy.o
MAN=	nslookup.8
FILES=	nslookup.help
FILESDIR=/usr/share/misc

.include "../Makefile.inc"
.include "../../Makefile.inc"
.include <bsd.prog.mk>
