#	$NetBSD: Makefile,v 1.30 2006/12/16 12:59:17 bouyer Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/27/95

.include <bsd.own.mk>

PROG=	mtree
#CPPFLAGS+=-DDEBUG
CPPFLAGS+= -DMTREE
MAN=	mtree.8
SRCS=	compare.c crc.c create.c excludes.c misc.c mtree.c spec.c verify.c \
	getid.c pack_dev.c
.if (${HOSTPROG:U} == "")
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif
WARNS?=	3

CPPFLAGS+=	-I${NETBSDSRCDIR}/sbin/mknod
.PATH:		${NETBSDSRCDIR}/sbin/mknod

.include <bsd.prog.mk>
