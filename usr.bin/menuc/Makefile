#	$NetBSD: Makefile,v 1.9 2001/11/12 20:30:43 tv Exp $

.include <bsd.own.mk>

PROG=		menuc
SRCS=		main.c parse.y scan.l avl.c mdb.c util.c
CPPFLAGS+=	-I. -I${.CURDIR}
YHEADER=

LDADD+=		-ll
DPADD+=		${LIBL}

.if ${MKSHARE} != "no"
FILES=		menu_sys.def
FILESDIR=	/usr/share/misc
.endif

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
