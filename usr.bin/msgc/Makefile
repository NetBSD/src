#	$NetBSD: Makefile,v 1.15 2003/09/25 18:32:10 dsl Exp $

.include <bsd.own.mk>

PROG=		msgc
SRCS=		msgmain.c msgparse.y msgscan.l msgdb.c util.c avl.c
.PATH:		${NETBSDSRCDIR}/usr.bin/menuc
CPPFLAGS+=	-I. -I${.CURDIR}
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
