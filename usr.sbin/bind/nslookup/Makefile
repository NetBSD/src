#	$NetBSD: Makefile,v 1.2.2.2 2000/12/13 23:57:45 he Exp $

.include "../Makefile.inc"
.PATH:	${BIND_DIST_DIR}/bin/nslookup \
	${BIND_DIST_DIR}/man

BINDIR=	/usr/bin
PROG=	nslookup
SRCS=	main.c getinfo.c debug.c send.c skip.c list.c subr.c commands.l
CPPFLAGS+= ${INCLUDE} -I${BIND_DIST_DIR}/bin/nslookup
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
