#	$NetBSD: Makefile,v 1.13 2009/04/20 16:05:30 drochner Exp $

WARNS?= 1	# XXX -Wshadow -Wcast-qual issues

.include <bsd.own.mk>

PROG=		menuc
SRCS=		main.c parse.y scan.l avl.c mdb.c util.c
CPPFLAGS+=	-I. -I${.CURDIR} -DYY_NO_INPUT
YHEADER=

.if ${MKSHARE} != "no"
FILES=		menu_sys.def
FILESDIR=	/usr/share/misc
.endif

.ifndef HOSTPROG
LDADD+=		-ll
DPADD+=		${LIBL}
.endif

.include <bsd.prog.mk>
