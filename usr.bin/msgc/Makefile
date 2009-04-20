#	$NetBSD: Makefile,v 1.17 2009/04/20 16:05:30 drochner Exp $

WARNS?= 1	# XXX -Wshadow -Wcast-qual issues

.include <bsd.own.mk>

PROG=		msgc
SRCS=		msgmain.c msgparse.y msgscan.l msgdb.c util.c avl.c
.PATH:		${NETBSDSRCDIR}/usr.bin/menuc
CPPFLAGS+=	-I. -I${.CURDIR} -DYY_NO_INPUT
YHEADER=

.if ${MKSHARE} != "no"
FILES=		msg_sys.def
FILESDIR=	/usr/share/misc
.endif

.ifndef HOSTPROG
LDADD+=		-ll
DPADD+=		${LIBL}
.endif

.include <bsd.prog.mk>
