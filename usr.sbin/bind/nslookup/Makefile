#	$NetBSD: Makefile,v 1.6 2000/07/04 20:27:36 matt Exp $

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
