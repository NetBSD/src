#	$NetBSD: Makefile,v 1.7 1997/10/25 06:58:28 lukem Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:48 vixie Exp 

.PATH:	${.CURDIR}/../nslookup \
	${.CURDIR}/../man

PROG=	nslookup
SRCS=	main.c getinfo.c debug.c send.c skip.c list.c subr.c commands.l
CPPFLAGS+= ${INCLUDE}
DPADD=	${LIBL}
LDADD=	-ll
CLEANFILES+=	commands.c lex.yy.c lex.yy.o
MAN=	nslookup.8
FILES=	nslookup.help
FILESDIR=/usr/share/misc

.include "../../Makefile.inc"
.include <bsd.prog.mk>
