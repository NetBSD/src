#	$NetBSD: Makefile,v 1.10 2002/01/31 22:43:55 tv Exp $

.include <bsd.own.mk>

PROG=		menuc
SRCS=		main.c parse.y scan.l avl.c mdb.c util.c
CPPFLAGS+=	-I. -I${.CURDIR}
YHEADER=

.if ${MKSHARE} != "no"
FILES=		menu_sys.def
FILESDIR=	/usr/share/misc
.endif

.ifndef HOSTPROG
LDADD+=		-ll
DPADD+=		${LIBL}

.include <bsd.prog.mk>
.endif
