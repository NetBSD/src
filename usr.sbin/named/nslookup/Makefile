#	$NetBSD: Makefile,v 1.9 1999/02/13 02:55:00 lukem Exp $
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
.if ${MKSHARE} != "no"
FILES=	nslookup.help
FILESDIR=/usr/share/misc
.endif

.include "../../Makefile.inc"
.include <bsd.prog.mk>
